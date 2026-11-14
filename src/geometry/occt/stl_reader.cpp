#include "stl_reader.hpp"

#include "geometry/occt/path.hpp"

#include <Message_ProgressScope.hxx>
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_WIN32)
// windows.h comes from windows_prelude.hpp (WIN32_LEAN_AND_MEAN).
#else
#include <atomic>
#include <csignal>
#include <fcntl.h>
#include <mutex>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace geometry::occt
{
    namespace
    {
        static_assert(std::endian::native == std::endian::little, "binary STL is little-endian");

        constexpr std::size_t kHeaderSize = 84; // 80-byte comment + uint32 triangle count
        constexpr std::size_t kFacetSize  = 50; // normal, 3 vertices (12 floats), uint16 attribute
        constexpr std::size_t kNormalSize = 12;
        constexpr std::size_t kVertexSize = 12;

        // A read-only view of a whole file.
        class MappedFile
        {
          public:
            explicit MappedFile(const std::filesystem::path& path)
            {
#if defined(_WIN32)
                _file = CreateFileW(path.wstring().c_str(), GENERIC_READ,
                                    // As permissive as the CRT's fopen (RWStl): another program
                                    // may hold the file open for writing (OneDrive, a slicer).
                                    // Truncating it is refused while it is mapped.
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
                LARGE_INTEGER size{};
                if (_file == INVALID_HANDLE_VALUE || !GetFileSizeEx(_file, &size))
                    fail(path);
                _size = static_cast<std::size_t>(size.QuadPart);
                if (_size == 0)
                    return;
                _mapping = CreateFileMappingW(_file, nullptr, PAGE_READONLY, 0, 0, nullptr);
                if (_mapping == nullptr)
                    fail(path);
                _data = static_cast<const unsigned char*>(MapViewOfFile(_mapping, FILE_MAP_READ, 0, 0, 0));
                if (_data == nullptr)
                    fail(path);
                _stamp = stamp();
#else
                _fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
                struct stat info{};
                if (_fd < 0 || ::fstat(_fd, &info) != 0)
                    fail(path);
                _size = static_cast<std::size_t>(info.st_size);
                if (_size == 0)
                    return;
                void* data = ::mmap(nullptr, _size, PROT_READ, MAP_PRIVATE, _fd, 0);
                if (data == MAP_FAILED)
                    fail(path);
                _data  = static_cast<const unsigned char*>(data);
                _stamp = stamp();
#endif
            }

            ~MappedFile()
            {
                release();
            }

            MappedFile(const MappedFile&)            = delete;
            MappedFile& operator=(const MappedFile&) = delete;

            [[nodiscard]] const unsigned char* data() const noexcept
            {
                return _data;
            }

            [[nodiscard]] std::size_t size() const noexcept
            {
                return _size;
            }

            /** @brief Whether another program changed the file (size or modification time) since it was mapped. */
            [[nodiscard]] bool changedSinceMapped() const noexcept
            {
                return stamp() != _stamp;
            }

          private:
            using Stamp = std::array<std::int64_t, 3>; // size, modification time (s, ns)

            [[nodiscard]] Stamp stamp() const noexcept
            {
#if defined(_WIN32)
                BY_HANDLE_FILE_INFORMATION info{};
                if (_file == INVALID_HANDLE_VALUE || !GetFileInformationByHandle(_file, &info))
                    return {-1, -1, -1};
                return {static_cast<std::int64_t>((static_cast<std::uint64_t>(info.nFileSizeHigh) << 32) | info.nFileSizeLow),
                        static_cast<std::int64_t>(info.ftLastWriteTime.dwHighDateTime),
                        static_cast<std::int64_t>(info.ftLastWriteTime.dwLowDateTime)};
#else
                struct stat info{};
                if (_fd < 0 || ::fstat(_fd, &info) != 0)
                    return {-1, -1, -1};
#if defined(__APPLE__)
                return {static_cast<std::int64_t>(info.st_size), static_cast<std::int64_t>(info.st_mtimespec.tv_sec),
                        static_cast<std::int64_t>(info.st_mtimespec.tv_nsec)};
#else
                return {static_cast<std::int64_t>(info.st_size), static_cast<std::int64_t>(info.st_mtim.tv_sec),
                        static_cast<std::int64_t>(info.st_mtim.tv_nsec)};
#endif
#endif
            }

            [[noreturn]] void fail(const std::filesystem::path& path)
            {
                release();
                throw std::runtime_error("Cannot read STL file: " + toUtf8(path));
            }

            void release() noexcept
            {
#if defined(_WIN32)
                if (_data != nullptr)
                    UnmapViewOfFile(_data);
                if (_mapping != nullptr)
                    CloseHandle(_mapping);
                if (_file != INVALID_HANDLE_VALUE)
                    CloseHandle(_file);
                _mapping = nullptr;
                _file    = INVALID_HANDLE_VALUE;
#else
                if (_data != nullptr)
                    ::munmap(const_cast<unsigned char*>(_data), _size);
                if (_fd >= 0)
                    ::close(_fd);
                _fd = -1;
#endif
                _data = nullptr;
            }

#if defined(_WIN32)
            HANDLE _file    = INVALID_HANDLE_VALUE;
            HANDLE _mapping = nullptr;
#else
            int _fd = -1;
#endif
            const unsigned char* _data = nullptr;
            std::size_t          _size = 0;
            Stamp                _stamp{};
        };

#if !defined(_WIN32)
        // Another program truncating the file while it is mapped makes reading
        // past the new end raise SIGBUS, which would kill the app. While a read
        // is in progress, a fault inside its mapping gets a zero page instead,
        // and the read reports that the file changed. Faults elsewhere go to
        // whatever handler was there before.
        class BusGuard
        {
          public:
            BusGuard(const unsigned char* data, std::size_t size)
            {
                const std::lock_guard lock(mutex());
                for (std::size_t i = 0; i < kSlots; ++i)
                {
                    std::uintptr_t expected = 0;
                    if (slots()[i].begin.compare_exchange_strong(expected, reinterpret_cast<std::uintptr_t>(data)))
                    {
                        _slot              = i;
                        slots()[i].end     = reinterpret_cast<std::uintptr_t>(data) + size;
                        slots()[i].faulted = false;
                        break;
                    }
                }
                if (users()++ == 0)
                {
                    pageSize() = static_cast<std::uintptr_t>(::sysconf(_SC_PAGESIZE));
                    struct sigaction action{};
                    action.sa_sigaction = &BusGuard::onBus;
                    action.sa_flags     = SA_SIGINFO;
                    sigemptyset(&action.sa_mask);
                    ::sigaction(SIGBUS, &action, &previous());
                }
            }

            ~BusGuard()
            {
                const std::lock_guard lock(mutex());
                if (_slot < kSlots)
                {
                    slots()[_slot].end   = 0;
                    slots()[_slot].begin = 0;
                }
                if (--users() == 0)
                    ::sigaction(SIGBUS, &previous(), nullptr);
            }

            BusGuard(const BusGuard&)            = delete;
            BusGuard& operator=(const BusGuard&) = delete;

            [[nodiscard]] bool faulted() const noexcept
            {
                return _slot < kSlots && slots()[_slot].faulted;
            }

          private:
            static constexpr std::size_t kSlots = 16; // concurrent reads guarded

            struct Slot
            {
                std::atomic<std::uintptr_t> begin{0};
                std::atomic<std::uintptr_t> end{0};
                std::atomic<bool>           faulted{false};
            };

            static std::array<Slot, kSlots>& slots() noexcept
            {
                static std::array<Slot, kSlots> instance;
                return instance;
            }

            static std::mutex& mutex() noexcept
            {
                static std::mutex instance;
                return instance;
            }

            static std::size_t& users() noexcept
            {
                static std::size_t instance = 0;
                return instance;
            }

            static std::uintptr_t& pageSize() noexcept
            {
                static std::uintptr_t instance = 4096;
                return instance;
            }

            static struct sigaction& previous() noexcept
            {
                static struct sigaction instance{};
                return instance;
            }

            static void onBus(int signal, siginfo_t* info, void* context)
            {
                const auto address = reinterpret_cast<std::uintptr_t>(info->si_addr);
                for (auto& slot : slots())
                {
                    const std::uintptr_t begin = slot.begin;
                    if (begin != 0 && address >= begin && address < slot.end)
                    {
                        // Back the vanished page with zeros and let the read go on.
                        const std::uintptr_t page = address & ~(pageSize() - 1);
                        ::mmap(reinterpret_cast<void*>(page), pageSize(), PROT_READ, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
                        slot.faulted = true;
                        return;
                    }
                }
                const struct sigaction& before = previous();
                if ((before.sa_flags & SA_SIGINFO) != 0 && before.sa_sigaction != nullptr)
                    before.sa_sigaction(signal, info, context);
                else if (before.sa_handler != SIG_DFL && before.sa_handler != SIG_IGN && before.sa_handler != nullptr)
                    before.sa_handler(signal);
                else if (before.sa_handler != SIG_IGN)
                {
                    ::signal(SIGBUS, SIG_DFL);
                    ::raise(SIGBUS);
                }
            }

            std::size_t _slot = kSlots;
        };
#endif

        using Key = std::array<std::uint32_t, 3>; // a vertex's float coordinates, bit for bit

        std::uint64_t mix(std::uint64_t value) noexcept // splitmix64 finalizer
        {
            value ^= value >> 30;
            value *= 0xbf58476d1ce4e5b9ULL;
            value ^= value >> 27;
            value *= 0x94d049bb133111ebULL;
            return value ^ (value >> 31);
        }

        std::uint64_t hashOf(const Key& key) noexcept
        {
            return mix((static_cast<std::uint64_t>(key[0]) << 32 | key[1]) ^ mix(key[2]));
        }

        // Splits [0, count) into `parts` contiguous ranges; part p is [begin(p), begin(p + 1)).
        struct Split
        {
            std::size_t count = 0;
            std::size_t parts = 1;

            [[nodiscard]] std::size_t begin(std::size_t part) const noexcept
            {
                return count / parts * part + std::min(part, count % parts);
            }
        };
    } // namespace

    bool isBinarySTL(const std::filesystem::path& path)
    {
        std::error_code ec;
        const auto      size = std::filesystem::file_size(path, ec);
        if (ec || size < kHeaderSize + kFacetSize)
            return false;
        std::ifstream file(path, std::ios::binary);
        char          count[4];
        if (!file.seekg(80).read(count, 4))
            return false;
        std::uint32_t triangles = 0;
        std::memcpy(&triangles, count, 4);
        return size == kHeaderSize + kFacetSize * static_cast<std::uintmax_t>(triangles);
    }

    occ::handle<Poly_Triangulation> readBinarySTL(const std::filesystem::path& path,
                                                  const Message_ProgressRange& progress,
                                                  const ParallelFor&           parallelFor)
    {
        const auto run = [&](std::size_t first, std::size_t last, const std::function<void(std::size_t)>& body) {
            if (parallelFor)
                parallelFor(first, last, body);
            else
                for (std::size_t i = first; i < last; ++i)
                    body(i);
        };

        const MappedFile file(path);
#if !defined(_WIN32)
        const BusGuard guard(file.data(), file.size());
#endif
        if (file.size() < kHeaderSize)
            throw std::runtime_error("Not a binary STL file: " + toUtf8(path));
        std::uint32_t declared = 0;
        std::memcpy(&declared, file.data() + 80, 4);
        if (file.size() != kHeaderSize + kFacetSize * static_cast<std::size_t>(declared))
            throw std::runtime_error("Truncated or malformed binary STL file: " + toUtf8(path));
        if (declared == 0)
            throw std::runtime_error("No triangle in STL file: " + toUtf8(path));

        const std::size_t triangles = declared;
        const std::size_t corners   = triangles * 3;
        // Poly_Triangulation counts nodes and triangles with int.
        if (corners > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::runtime_error("STL file too large: " + toUtf8(path));

        Message_ProgressScope scope(progress, "Read binary STL", 4);
        const auto            checkCancel = [&] {
            if (scope.UserBreak())
                throw std::runtime_error("STL import cancelled: " + toUtf8(path));
        };
        const unsigned char* records = file.data() + kHeaderSize;
        const auto           keyOf   = [records](std::size_t corner) {
            Key key;
            std::memcpy(key.data(),
                        records + corner / 3 * kFacetSize + kNormalSize + corner % 3 * kVertexSize,
                        kVertexSize);
            // -0.0 is the same point as 0.0: one vertex, so the mesh (and the
            // B-Rep built from it) has no seam there. RWStl keeps them apart.
            for (auto& bits : key)
                if (bits == 0x80000000u)
                    bits = 0;
            return key;
        };

        // 1. Scatter corners into hash buckets: equal vertices share a bucket.
        //    Each chunk counts, then writes, its corners at precomputed offsets.
        const std::size_t          buckets = std::clamp<std::size_t>(corners / 2048, 1, 1 << 14);
        const Split                chunks{corners, std::min<std::size_t>(256, std::max<std::size_t>(1, corners / 4096))};
        std::vector<std::uint32_t> counts(chunks.parts * buckets, 0);
        run(0, chunks.parts, [&](std::size_t chunk) {
            std::uint32_t* row = counts.data() + chunk * buckets;
            for (std::size_t c = chunks.begin(chunk); c < chunks.begin(chunk + 1); ++c)
                ++row[hashOf(keyOf(c)) % buckets];
        });
        std::vector<std::size_t> bucketStart(buckets + 1, 0);
        std::vector<std::size_t> offsets(chunks.parts * buckets);
        {
            std::size_t position = 0;
            for (std::size_t bucket = 0; bucket < buckets; ++bucket)
            {
                bucketStart[bucket] = position;
                for (std::size_t chunk = 0; chunk < chunks.parts; ++chunk)
                {
                    offsets[chunk * buckets + bucket] = position;
                    position += counts[chunk * buckets + bucket];
                }
            }
            bucketStart[buckets] = position;
        }
        counts = {};
        std::vector<std::uint32_t> order(corners);
        run(0, chunks.parts, [&](std::size_t chunk) {
            std::size_t* next = offsets.data() + chunk * buckets;
            for (std::size_t c = chunks.begin(chunk); c < chunks.begin(chunk + 1); ++c)
                order[next[hashOf(keyOf(c)) % buckets]++] = static_cast<std::uint32_t>(c);
        });
        offsets = {};
        checkCancel();
        scope.Next();

        // 2. In each bucket, sort by (vertex, corner): every corner points at
        //    the first corner (in file order) with the same vertex.
        std::vector<std::uint32_t> first(corners);
        run(0, buckets, [&](std::size_t bucket) {
            auto* begin = order.data() + bucketStart[bucket];
            auto* end   = order.data() + bucketStart[bucket + 1];
            std::sort(begin, end, [&](std::uint32_t a, std::uint32_t b) {
                const Key ka = keyOf(a);
                const Key kb = keyOf(b);
                return ka != kb ? ka < kb : a < b;
            });
            for (auto* group = begin; group != end;)
            {
                const Key key  = keyOf(*group);
                auto*     stop = group;
                while (stop != end && keyOf(*stop) == key)
                    first[*stop++] = *group;
                group = stop;
            }
        });
        checkCancel();
        scope.Next();

        // 3. Number the nodes in order of first appearance (RWStl's order),
        //    then give every corner its node (reusing `order`).
        std::vector<std::uint32_t> firstsBefore(chunks.parts + 1, 0);
        run(0, chunks.parts, [&](std::size_t chunk) {
            std::uint32_t count = 0;
            for (std::size_t c = chunks.begin(chunk); c < chunks.begin(chunk + 1); ++c)
                if (first[c] == c)
                    ++count;
            firstsBefore[chunk + 1] = count;
        });
        std::partial_sum(firstsBefore.begin(), firstsBefore.end(), firstsBefore.begin());
        const std::size_t           nodes  = firstsBefore.back();
        std::vector<std::uint32_t>& nodeOf = order;
        run(0, chunks.parts, [&](std::size_t chunk) {
            std::uint32_t node = firstsBefore[chunk];
            for (std::size_t c = chunks.begin(chunk); c < chunks.begin(chunk + 1); ++c)
                if (first[c] == c)
                    nodeOf[c] = ++node; // 1-based, as Poly_Triangulation
        });
        run(0, chunks.parts, [&](std::size_t chunk) {
            for (std::size_t c = chunks.begin(chunk); c < chunks.begin(chunk + 1); ++c)
                if (first[c] != c)
                    nodeOf[c] = nodeOf[first[c]];
        });
        checkCancel();
        scope.Next();

        // 4. Keep the triangles whose three nodes differ, in file order, and
        //    fill a single-precision triangulation.
        const Split                triangleChunks{triangles, chunks.parts};
        std::vector<std::uint32_t> keptBefore(triangleChunks.parts + 1, 0);
        const auto                 kept = [&](std::size_t t) {
            const std::uint32_t a = nodeOf[3 * t];
            const std::uint32_t b = nodeOf[3 * t + 1];
            const std::uint32_t c = nodeOf[3 * t + 2];
            return a != b && b != c && a != c;
        };
        run(0, triangleChunks.parts, [&](std::size_t chunk) {
            std::uint32_t count = 0;
            for (std::size_t t = triangleChunks.begin(chunk); t < triangleChunks.begin(chunk + 1); ++t)
                if (kept(t))
                    ++count;
            keptBefore[chunk + 1] = count;
        });
        std::partial_sum(keptBefore.begin(), keptBefore.end(), keptBefore.begin());
        if (keptBefore.back() == 0)
            throw std::runtime_error("No triangle in STL file: " + toUtf8(path));

        occ::handle<Poly_Triangulation> mesh = new Poly_Triangulation();
        mesh->SetDoublePrecision(false); // STL stores floats: nothing is lost
        mesh->ResizeNodes(static_cast<int>(nodes), false);
        mesh->ResizeTriangles(static_cast<int>(keptBefore.back()), false);
        run(0, chunks.parts, [&](std::size_t chunk) {
            for (std::size_t c = chunks.begin(chunk); c < chunks.begin(chunk + 1); ++c)
            {
                if (first[c] != c)
                    continue;
                const Key key = keyOf(c);
                float     xyz[3];
                std::memcpy(xyz, key.data(), sizeof(xyz));
                mesh->SetNode(static_cast<int>(nodeOf[c]), gp_Pnt(static_cast<double>(xyz[0]), static_cast<double>(xyz[1]), static_cast<double>(xyz[2])));
            }
        });
        run(0, triangleChunks.parts, [&](std::size_t chunk) {
            int index = static_cast<int>(keptBefore[chunk]);
            for (std::size_t t = triangleChunks.begin(chunk); t < triangleChunks.begin(chunk + 1); ++t)
                if (kept(t))
                    mesh->SetTriangle(++index,
                                      Poly_Triangle(static_cast<int>(nodeOf[3 * t]), static_cast<int>(nodeOf[3 * t + 1]),
                                                    static_cast<int>(nodeOf[3 * t + 2])));
        });
        checkCancel();
#if !defined(_WIN32)
        const bool faulted = guard.faulted();
#else
        const bool faulted = false;
#endif
        if (faulted || file.changedSinceMapped())
            throw std::runtime_error("STL file changed while it was imported: " + toUtf8(path));
        scope.Next();
        return mesh;
    }
} // namespace geometry::occt

#include "geometry/occt/shape.hpp"
#include "geometry/occt/stl_reader.hpp"
#include "platform/threading/dispatcher.hpp"
#include "support/progress_probe.hpp"
#include "support/temp_dir.hpp"

#include <RWStl.hxx>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using geometry::occt::isBinarySTL;
using geometry::occt::readBinarySTL;

namespace
{
    const std::filesystem::path kBinary = std::filesystem::path(SPLINTER3D_TEST_ASSETS_DIR) / "stl" / "binary";

    // The Dispatcher's parallelFor, as the import task passes it.
    void dispatcherParallelFor(std::size_t first, std::size_t last, const std::function<void(std::size_t)>& body)
    {
        platform::threading::Dispatcher::getInstance().parallelFor(first, last, body);
    }

    std::vector<char> readAll(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }

    void writeAll(const std::filesystem::path& path, const std::vector<char>& bytes)
    {
        std::ofstream(path, std::ios::binary).write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }

    // Same nodes, in the same order, and the same triangles as RWStl.
    void requireSameAsRWStl(const occ::handle<Poly_Triangulation>& mesh, const std::filesystem::path& path)
    {
        const auto expected = RWStl::ReadFile(path.string().c_str());
        REQUIRE_FALSE(expected.IsNull());
        REQUIRE(mesh->NbNodes() == expected->NbNodes());
        REQUIRE(mesh->NbTriangles() == expected->NbTriangles());
        for (int i = 1; i <= mesh->NbNodes(); ++i)
            REQUIRE(mesh->Node(i).Distance(expected->Node(i)) == 0.0);
        for (int i = 1; i <= mesh->NbTriangles(); ++i)
        {
            int a[3];
            int b[3];
            mesh->Triangle(i).Get(a[0], a[1], a[2]);
            expected->Triangle(i).Get(b[0], b[1], b[2]);
            REQUIRE((a[0] == b[0] && a[1] == b[1] && a[2] == b[2]));
        }
    }
} // namespace

TEST_CASE("readBinarySTL: matches RWStl node for node", "[geometry][stl]")
{
    const std::string file = GENERATE("cube.stl", "magnolia_binary.stl");
    CAPTURE(file);

    SECTION("sequentially")
    {
        const auto mesh = readBinarySTL(kBinary / file);
        requireSameAsRWStl(mesh, kBinary / file);
        CHECK_FALSE(mesh->IsDoublePrecision());
    }
    SECTION("on the Dispatcher's workers")
    {
        occ::handle<Poly_Triangulation> mesh;
        platform::threading::Dispatcher::getInstance().run(
            "test.stl", [&] { mesh = readBinarySTL(kBinary / file, {}, dispatcherParallelFor); });
        requireSameAsRWStl(mesh, kBinary / file);
    }
}

TEST_CASE("isBinarySTL: by size, as the header's count implies", "[geometry][stl]")
{
    CHECK(isBinarySTL(kBinary / "cube.stl"));
    CHECK(isBinarySTL(kBinary / "magnolia_binary.stl"));
    CHECK_FALSE(isBinarySTL(kBinary / "model.stl")); // ASCII, despite its folder
    CHECK_FALSE(isBinarySTL(kBinary.parent_path() / "ascii" / "cube.stl"));
    CHECK_FALSE(isBinarySTL(kBinary / "missing.stl"));
}

TEST_CASE("readBinarySTL: a binary header starting with 'solid' is still binary", "[geometry][stl]")
{
    const tests::support::TempDir dir;
    auto                          bytes  = readAll(kBinary / "cube.stl");
    const std::string             header = "solid fake ascii-looking header";
    std::copy(header.begin(), header.end(), bytes.begin());
    writeAll(dir.path() / "solid.stl", bytes);

    REQUIRE(isBinarySTL(dir.path() / "solid.stl"));
    CHECK(readBinarySTL(dir.path() / "solid.stl")->NbTriangles() == 12);
    CHECK(geometry::occt::Shape::fromSTL(dir.path() / "solid.stl").triangleCount() == 12);
}

TEST_CASE("readBinarySTL: a truncated file throws", "[geometry][stl]")
{
    const tests::support::TempDir dir;
    auto                          bytes = readAll(kBinary / "cube.stl");
    bytes.resize(bytes.size() - 10);
    writeAll(dir.path() / "cut.stl", bytes);

    CHECK_FALSE(isBinarySTL(dir.path() / "cut.stl"));
    REQUIRE_THROWS_AS(readBinarySTL(dir.path() / "cut.stl"), std::runtime_error);
}

TEST_CASE("readBinarySTL: reports progress and stops when cancelled", "[geometry][stl]")
{
    occ::handle<tests::support::ProgressProbe> probe = new tests::support::ProgressProbe();
    (void) readBinarySTL(kBinary / "magnolia_binary.stl", probe->Start());
    CHECK(probe->maxPosition >= 0.99);

    occ::handle<tests::support::ProgressProbe> cancelled = new tests::support::ProgressProbe();
    cancelled->cancel                                    = true;
    REQUIRE_THROWS_AS(readBinarySTL(kBinary / "magnolia_binary.stl", cancelled->Start()), std::runtime_error);
}

TEST_CASE("Shape::fromSTL: ASCII and odd files still go through RWStl", "[geometry][stl]")
{
    CHECK(geometry::occt::Shape::fromSTL(kBinary / "model.stl").triangleCount() ==
          RWStl::ReadFile((kBinary / "model.stl").string().c_str())->NbTriangles());
    CHECK(geometry::occt::Shape::fromSTL(kBinary.parent_path() / "ascii" / "cube.stl").triangleCount() == 12);
}

namespace
{
    // A binary STL big enough for many chunks and hash buckets: triangles
    // drawn from a small vertex pool (so vertices repeat across chunk
    // boundaries), with some degenerate ones (two equal corners).
    void writeGeneratedSTL(const std::filesystem::path& path, std::uint32_t triangles)
    {
        std::mt19937                       random(1234);
        std::uniform_int_distribution<int> coordinate(-500, 500);
        std::vector<std::array<float, 3>>  pool(triangles / 8);
        for (auto& vertex : pool)
            for (auto& value : vertex)
                value = static_cast<float>(coordinate(random)) / 8.0f;
        std::uniform_int_distribution<std::size_t> pick(0, pool.size() - 1);
        std::uniform_int_distribution<int>         degenerate(0, 49);

        std::vector<char> bytes(84 + 50 * static_cast<std::size_t>(triangles), 0);
        std::memcpy(bytes.data() + 80, &triangles, 4);
        for (std::uint32_t t = 0; t < triangles; ++t)
        {
            const std::size_t a      = pick(random);
            const std::size_t b      = pick(random);
            const std::size_t c      = degenerate(random) == 0 ? a : pick(random);
            char*             record = bytes.data() + 84 + 50 * static_cast<std::size_t>(t);
            std::memcpy(record + 12, pool[a].data(), 12);
            std::memcpy(record + 24, pool[b].data(), 12);
            std::memcpy(record + 36, pool[c].data(), 12);
        }
        writeAll(path, bytes);
    }
} // namespace

TEST_CASE("readBinarySTL: a large file split across chunks and buckets matches RWStl", "[geometry][stl]")
{
    const tests::support::TempDir dir;
    const auto                    path = dir.path() / "generated.stl";
    writeGeneratedSTL(path, 60000); // 180,000 corners: dozens of chunks and buckets
    REQUIRE(isBinarySTL(path));

    SECTION("sequentially")
    {
        requireSameAsRWStl(readBinarySTL(path), path);
    }
    SECTION("on the Dispatcher's workers")
    {
        occ::handle<Poly_Triangulation> mesh;
        platform::threading::Dispatcher::getInstance().run(
            "test.stl", [&] { mesh = readBinarySTL(path, {}, dispatcherParallelFor); });
        requireSameAsRWStl(mesh, path);
    }
}

TEST_CASE("readBinarySTL: +0.0 and -0.0 are the same vertex", "[geometry][stl]")
{
    // Two triangles sharing an edge, one written with -0.0: one shared edge,
    // so the B-Rep built from it later is closed there.
    const tests::support::TempDir dir;
    const float                   negativeZero     = -0.0f;
    const float                   corners[2][3][3] = {{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}},
                                                      {{1, 0, 0}, {negativeZero, 1, 0}, {1, 1, 0}}};
    std::vector<char>             bytes(84 + 50 * 2, 0);
    const std::uint32_t           count = 2;
    std::memcpy(bytes.data() + 80, &count, 4);
    for (std::size_t t = 0; t < 2; ++t)
        std::memcpy(bytes.data() + 84 + 50 * t + 12, corners[t], sizeof(corners[t]));
    writeAll(dir.path() / "zeros.stl", bytes);

    const auto mesh = readBinarySTL(dir.path() / "zeros.stl");
    CHECK(mesh->NbNodes() == 4);
}

TEST_CASE("readBinarySTL: a file changed while it is read fails cleanly", "[geometry][stl]")
{
    const tests::support::TempDir dir;
    const auto                    path = dir.path() / "changing.stl";
    writeAll(path, readAll(kBinary / "magnolia_binary.stl"));

    // Truncate the file once the read has started, as another program could.
    bool truncated  = false;
    auto truncating = [&](std::size_t first, std::size_t last, const std::function<void(std::size_t)>& body) {
        if (!truncated)
        {
            truncated = true;
            std::filesystem::resize_file(path, 100);
        }
        for (std::size_t i = first; i < last; ++i)
            body(i);
    };
    REQUIRE_THROWS_AS(readBinarySTL(path, {}, truncating), std::runtime_error);

    // The app is still fine: a later read of a good file works.
    CHECK(readBinarySTL(kBinary / "cube.stl")->NbTriangles() == 12);
}

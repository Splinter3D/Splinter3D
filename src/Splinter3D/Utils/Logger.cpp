#include <Splinter3D/Utils/Logger.hpp>

namespace splinter3D::utils
{

    void Logger::_maybeStartThreads(std::ostream& oss, const std::string& msg)
    {
        if (!_outFile.has_value())
        {
            _printToConsole(oss, msg);
            return;
        }
        std::thread t1(&Logger::_printToFile, this, std::cref(msg));
        std::thread t2(&Logger::_printToConsole, this, std::ref(oss), std::cref(msg));

        t1.join();
        t2.join();
    }

    void Logger::_printToFile(const std::string& msg)
    {
        if (_outFile.has_value())
        {
            std::lock_guard<std::mutex> lock(_fileMtx);
            _outFile.value() << msg << std::endl;
        }
    }

    void Logger::_printToConsole(std::ostream& oss, const std::string& msg)
    {
        std::lock_guard<std::mutex> lock(_consoleMtx);
        oss << msg << std::endl;
    }

} // namespace splinter3D::utils

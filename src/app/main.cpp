/**
 * @file main.cpp
 * @brief Application entry point.
 *
 * Hands the command line to batch mode first (app/cli/batch_mode.hpp), which
 * must run before wxWidgets is initialized so it needs no display; otherwise
 * starts the GUI through wxEntry, exactly as wxIMPLEMENT_APP's generated
 * entry point would. No other logic should be added here - see app/README.md
 * for the dependency rule.
 */

#include "cli/batch_mode.hpp"
#include "wx/wx_app.hpp"

#include <string>
#include <vector>

wxIMPLEMENT_APP_NO_MAIN(app::WxApp);

#if defined(_WIN32)

// wx's own <windows.h> wrapper (keeps min/max macros out), which shellapi.h
// (CommandLineToArgvW) needs to come first.
#include <wx/msw/wrapwin.h>
// clang-format off
#include <shellapi.h>
// clang-format on

namespace
{
    // WinMain receives no argv; rebuild it as UTF-8 from the wide command line.
    std::vector<std::string> utf8CommandLine()
    {
        std::vector<std::string> args;
        int                      argc = 0;
        LPWSTR*                  argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (argv == nullptr)
            return args;
        for (int i = 0; i < argc; ++i)
        {
            const int   size = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
            std::string arg(static_cast<std::size_t>(size > 0 ? size - 1 : 0), '\0');
            if (size > 1)
                WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, arg.data(), size, nullptr, nullptr);
            args.push_back(std::move(arg));
        }
        LocalFree(argv);
        return args;
    }
} // namespace

// Same as the WinMain wxIMPLEMENT_APP generates (wx/msw/app.h), plus batch mode.
extern "C" int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wxCmdLineArgType, int nCmdShow)
{
    if (const auto exitCode = app::cli::runBatchIfRequested(utf8CommandLine()))
        return *exitCode;

    wxDISABLE_DEBUG_SUPPORT();
    return wxEntry(hInstance, hPrevInstance, nullptr, nCmdShow);
}

#else

int main(int argc, char** argv)
{
    if (const auto exitCode = app::cli::runBatchIfRequested(std::vector<std::string>(argv, argv + argc)))
        return *exitCode;

    wxDISABLE_DEBUG_SUPPORT();
    return wxEntry(argc, argv);
}

#endif

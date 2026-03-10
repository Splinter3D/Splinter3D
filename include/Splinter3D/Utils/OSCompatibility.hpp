/*
 * Cross-platform OS compatibility helpers
 * - disable/restore terminal echo for Ctrl-C display
 * - install simple signal/console handlers that set an atomic flag
 */

#pragma once

// Platform detection
#if !defined(_WIN32) && !defined(_WIN64)
#define SPLINTER3D_POSIX 1
#else
#define SPLINTER3D_WINDOWS 1
#endif

#include <atomic>
#include <functional>

// POSIX headers
#if defined(SPLINTER3D_POSIX)
#include <cstdlib>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#endif

// Minimal C headers needed for Windows shims
#if defined(SPLINTER3D_WINDOWS)
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#endif

namespace splinter3D::utils::oscompat
{

    // Signal flag set when a console interrupt/close is requested
    inline std::atomic<bool> s_signalReceived{false};

    // Install platform signal/console handlers that set s_signalReceived
    inline void InstallSignalHandlers()
    {
#if defined(SPLINTER3D_POSIX)
        signal(SIGINT, +[](int s) { (void)s; s_signalReceived.store(true); });
        signal(SIGTERM, +[](int s) { (void)s; s_signalReceived.store(true); });
#else
        // no-op on Windows to avoid including <windows.h> here
#endif
    }

    // Query whether a signal was received
    inline bool SignalReceived()
    {
        return s_signalReceived.load();
    }

    // Clear the signal flag
    inline void ClearSignal()
    {
        s_signalReceived.store(false);
    }

    // Terminal echo control (to hide control character echo like ^C on POSIX)
#if defined(SPLINTER3D_POSIX)
    inline void disableCtrlCEcho()
    {
        static struct termios origTerm;
        if (tcgetattr(STDIN_FILENO, &origTerm) == 0)
        {
            atexit([]() { tcsetattr(STDIN_FILENO, TCSANOW, &origTerm); });
            struct termios newTerm = origTerm;
#ifdef ECHOCTL
            newTerm.c_lflag &= ~static_cast<tcflag_t>(ECHOCTL);
#endif
            tcsetattr(STDIN_FILENO, TCSANOW, &newTerm);
        }
    }

    inline void restoreTerminal()
    {
        // atexit handler in disableCtrlCEcho will restore terminal
    }
#else
    // On Windows make these no-ops here to avoid requiring a separate .cpp
    inline void disableCtrlCEcho()
    { }

    inline void restoreTerminal()
    { }
#endif

} // namespace splinter3D::utils::oscompat

// Provide global POSIX shims on Windows so unqualified 'setenv' and
// 'vasprintf' calls compile. These are placed in the global C linkage
// because existing code calls them unqualified.
#if defined(SPLINTER3D_WINDOWS)
extern "C" {
static inline int setenv(const char* name, const char* value, int overwrite)
{
    if (!overwrite)
    {
        char*  buf = nullptr;
        size_t len = 0;
        if (_dupenv_s(&buf, &len, name) == 0 && buf != nullptr)
        {
            free(buf);
            return 0;
        }
    }
    return _putenv_s(name, value) == 0 ? 0 : -1;
}

static inline int unsetenv(const char* name)
{
    return _putenv_s(name, "") == 0 ? 0 : -1;
}

static inline int vasprintf(char** strp, const char* fmt, va_list ap)
{
    if (strp == NULL || fmt == NULL)
        return -1;
    int len = _vscprintf(fmt, ap);
    if (len < 0)
        return -1;
    char* buf = (char*) malloc((size_t) len + 1);
    if (!buf)
        return -1;
    int rc = vsnprintf_s(buf, (size_t) len + 1, _TRUNCATE, fmt, ap);
    if (rc < 0)
    {
        free(buf);
        return -1;
    }
    *strp = buf;
    return rc;
}
} // extern "C"
#endif

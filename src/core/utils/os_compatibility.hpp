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

namespace core::utils
{

    /**
     * @brief Flag set to true when a console interrupt or close request
     * is received (POSIX only).
     *
     * Query via SignalReceived(), reset via ClearSignal().
     */
    inline std::atomic<bool> s_signalReceived{false};

    /**
     * @brief Installs signal handlers for graceful shutdown on interrupt.
     *
     * On POSIX, installs handlers for SIGINT and SIGTERM that set
     * s_signalReceived. On Windows, this is currently a no-op (kept
     * deliberately minimal to avoid pulling in <windows.h> here).
     */
    inline void InstallSignalHandlers()
    {
#if defined(SPLINTER3D_POSIX)
        signal(SIGINT, +[](int s) { (void)s; s_signalReceived.store(true); });
        signal(SIGTERM, +[](int s) { (void)s; s_signalReceived.store(true); });
#else
        // no-op on Windows to avoid including <windows.h> here
#endif
    }

    /**
     * @brief Checks whether an interrupt/termination signal was received.
     *
     * @return true if a signal was caught since the last call to
     * ClearSignal(), false otherwise.
     */
    inline bool SignalReceived()
    {
        return s_signalReceived.load();
    }

    /**
     * @brief Resets the signal flag.
     *
     * Should be called after handling a received signal, if the
     * application wishes to keep monitoring for further signals.
     */
    inline void ClearSignal()
    {
        s_signalReceived.store(false);
    }

    // Terminal echo control (to hide control character echo like ^C on POSIX)
#if defined(SPLINTER3D_POSIX)
    /**
     * @brief Disables terminal echo of control characters (e.g. "^C").
     *
     * Only affects POSIX terminals attached to standard input. Restores
     * the original terminal settings automatically at process exit via
     * atexit(). No-op if stdin is not a TTY.
     */
    inline void disableCtrlCEcho()
    {
        static struct termios origTerm;
        if (!isatty(STDIN_FILENO))
            return;
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

    /**
     * @brief Restores the terminal to its original state.
     *
     * On POSIX, the actual restoration happens automatically via the
     * atexit() handler registered in disableCtrlCEcho(); this function
     * exists mainly for symmetry and explicit call sites.
     */
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

} // namespace core::utils

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

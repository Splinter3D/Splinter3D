#pragma once

/**
 * @file windows_prelude.hpp
 * @brief Includes <windows.h> before any OpenCascade header, on Windows only.
 *
 * When <windows.h> has not been included yet, OpenCascade's Standard_Macro.hxx
 * defines NOGDI, NOUSER, NOMSG... and then includes it, which strips the GDI
 * and USER declarations (PIXELFORMATDESCRIPTOR, ...) from <windows.h> for the
 * rest of the file. Any wxWidgets or OpenGL header included after an OCCT one
 * then fails to compile (e.g. wx/msw/glcanvas.h: "syntax error: identifier
 * 'PIXELFORMATDESCRIPTOR'"). OCCT's own advice is to include <windows.h> first.
 *
 * Every header of the geometry/occt boundary that includes OCCT headers must
 * include this one before them.
 */

#if defined(_WIN32)
// Same as OCCT would define, minus its NOGDI/NOUSER/...: keeps GDI (wingdi.h),
// but leaves out legacy extras - notably <winsock.h> (Winsock 1), which clashes
// with the <winsock2.h> wxWidgets includes later ("'sockaddr': 'struct' type
// redefinition"). Files that need shellapi.h & co. include them explicitly.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX // no min()/max() macros (also what wxWidgets and OCCT ask for)
#endif
#include <windows.h>
#endif

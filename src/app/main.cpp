/**
 * @file main.cpp
 * @brief Application entry point.
 *
 * Delegates entirely to wxIMPLEMENT_APP, which instantiates WxApp and
 * generates the platform-specific entry point (WinMain on Windows,
 * main on Linux/macOS). No logic should ever be added here — see
 * app/README.md for the dependency rule.
 */

#include "wx/wx_app.hpp"

wxIMPLEMENT_APP(app::WxApp);

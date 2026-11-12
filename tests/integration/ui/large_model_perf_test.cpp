#include "platform/logger/logger.hpp"
#include "support/wx_helpers.hpp"
#include "ui/framework/occt/model_viewer_panel.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <wx/sizer.h>

// Hidden ([.]): imports and displays a large model in a real viewer and prints
// the timings. Run it by hand, e.g.
//   SPLINTER3D_LARGE_STL=assets/stl/binary/lucy.stl build/bin/splinter3D_integration_tests "[perf]"
// It never runs in CTest (hidden tests are not discovered).

TEST_CASE("Performance: importing and displaying a large model", "[.][perf]")
{
    const char* file = std::getenv("SPLINTER3D_LARGE_STL");
    if (file == nullptr || !std::filesystem::exists(file))
        SKIP("set SPLINTER3D_LARGE_STL to an STL file");

    tests::support::ScopedFrame<> frame(nullptr, wxID_ANY, "Large model", wxDefaultPosition, wxSize(1024, 768));
    auto*                         panel = new ui::framework::occt::ModelViewerPanel(frame.get());
    auto*                         sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(panel, 1, wxEXPAND);
    frame->SetSizer(sizer);
    frame.showAndWait();

    std::ostringstream captured;
    std::streambuf*    previous = std::clog.rdbuf(captured.rdbuf());
    platform::logger::logger::setDebug(true);

    const auto start = std::chrono::steady_clock::now();
    REQUIRE(panel->loadFile(file));
    const bool done = tests::support::pumpUntil([&] { return panel->tasks().idle(); }, std::chrono::minutes(10));
    const auto end  = std::chrono::steady_clock::now();

    platform::logger::logger::setDebug(false);
    std::clog.rdbuf(previous);
    REQUIRE(done);
    REQUIRE(panel->hasModel());

    std::istringstream lines(captured.str());
    for (std::string line; std::getline(lines, line);)
        if (line.find("[scene] imported") != std::string::npos || line.find("[viewer] displayed") != std::string::npos)
            std::cout << line << '\n';
    std::cout << "[perf] loadFile -> shown: " << std::chrono::duration<double>(end - start).count() << " s\n";
}

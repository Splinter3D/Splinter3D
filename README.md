# BS WxWidgets

## How to install

```bash
sudo apt install libwxgtk3.0-gtk3-dev
```

## How to build

```bash
g++ Main.cpp   Wx/Events/BindBasic.cpp Wx/Events/BindEvents.cpp   Wx/Events/BindFont.cpp  Wx/Events/BindLang.cpp   Wx/Utils/SwitchLanguage.cpp   `wx-config --cxxflags --libs`   -lwx_gtk3u_gl-3.2   $(pkg-config --cflags --libs gtk+-3.0)   -I/usr/include/opencascade   -lTKernel -lTKMath -lTKBRep -lTKTopAlgo -lTKMesh   -lTKSTEP -lTKSTL -lTKXSBase   -lTKV3d -lTKOpenGl -lTKService   -lTKPrim   -lGL -lGLU   -std=c++17 -o splinter3D
```
(didn't wanted to create a Makefile for this, but if you want to, go ahead)

## Architecture

```
.
├── Main.cpp                    // Main entry point of the application
├── Panels/                     // Directory containing panel classes for the BS app (used only as exemple)
├── splinter3D.cfg              // Configuration file created by the application
├── Wx/                         // WxWidgets wrapper
│   ├── Config/                 // Configuration management for the application (creates splinter3D.cfg)
│   ├── Events/                 // Directory of custom event handling for the application (binding of Font/Lang changes etc.)
│   ├── Locale/                 // Localization management for the application (handles translations)
│   ├── Occt/                   // Directory where we have the 3D rendering
│   ├── Theme/                  // Theme management for the application (handles colors, fonts, etc.)
│   ├── Utils/                  // Utils management for the application (for now, only the switch language handling)
│   ├── Widgets/                // Custom widget classes for the application (e.g., buttons, sliders, etc.)
│   │   ├── Button.hpp
│   │   ├── CheckRadio.hpp
│   │   ├── Dialogs.hpp
│   │   ├── Lists.hpp
│   │   ├── Slider.hpp
│   │   └── TextInput.hpp
│   ├── Ids.hpp                 // File where Ids are stored as enums (Ids are used to bind widgets on events)
│   └── Wx.hpp                  // Main header file for the WxWidgets wrapper
└── Locale                      // Directory for translations (.mo -> compiled translations, .po -> source translations)
    ├── de
    │   └── LC_MESSAGES
    │       ├── splinter3D.mo
    │       └── splinter3D.po
    ├── es
    │   └── LC_MESSAGES
    │       ├── splinter3D.mo
    │       └── splinter3D.po
    └── fr
        └── LC_MESSAGES
            ├── splinter3D.mo
            └── splinter3D.po
```

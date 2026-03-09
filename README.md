# BS WxWidgets

## How to install

```bash
sudo apt install libwxgtk3.0-gtk3-dev
```

## How to build

```bash
g++ Main.cpp `wx-config --cxxflags --libs`
```

## Architecture

```
.
├── Main.cpp                    // Main entry point of the application
├── Panels                      // Directory containing panel classes for the BS app (used only as exemple)
├── splinter3D.cfg              // Configuration file created by the application
├── Wx                          // WxWidgets wrapper
│   ├── Config                  // Configuration management for the application (creates splinter3D.cfg)
│   │   └── Config.hpp
│   ├── Locale                  // Localization management for the application (handles translations)
│   │   └── Locale.hpp
│   ├── Theme                   // Theme management for the application (handles colors, fonts, etc.)
│   │   └── Theme.hpp
│   ├── Widgets                 // Custom widget classes for the application (e.g., buttons, sliders, etc.)
│   │   ├── Button.hpp
│   │   ├── CheckRadio.hpp
│   │   ├── Dialogs.hpp
│   │   ├── Lists.hpp
│   │   ├── Slider.hpp
│   │   └── TextInput.hpp
│   ├── Events                  // Directory of custom event handling for the application (binding of Font/Lang changes etc.)
│   └── Wx.hpp                  // Main header file for the WxWidgets wrapper
└── locale                      // Directory for different translations (.mo -> compiled translations, .po -> source translations)
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
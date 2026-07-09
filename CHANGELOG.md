## v0.1.1 (2026-07-09)

### Fix

- **ci**: call release workflow after tag

## v0.1.0 (2026-07-09)

### Feat

- locale checking script
- **ci**: add github release workflows
- docker image build
- **ci**: multiarch build
- adding Visual Studio 18 2026 support
- custom tranlation
- adding a proper gestion of the user click when selecting an object
- docker image
- custom tranlation
- docker image
- use generic build script in ci
- macOS/Linux build script
- **wip**: base build script
- adding a proper gestion of the user click when selecting an object
- adding permissions to write pr
- adding checkout before gh pr close
- adding the secrets in the gh pr close
- removing the delete branch option
- using gh pr close instead of peter evans close action
- adding a check to avoid PR from other source that dev or hotfix/* onto main
- adding build_only to reusable build
- adding compilations checks on Pr onto main
- adding VSVersion parameter and passing DevMode to package in windows build
- adding color output for better cut rendering
- addding the cut using the button
- change app name to Splinter3D and link Applications folder

### Fix

- avoid ci on release-main branch main
- next tag when no initial commit was pushed
- **ci**: do not use an environment for builds
- make dependabot target dev
- add missing headers in RayGUI.hpp
- remove key sorting for json autoformatter
- ping clang-format version in pre-commit
- use matrix to build docker images instead of qemu
- **ci**: fixed refs and tags for reusable build and docker
- **ci**: fixed windows executable location and ubuntu dependencies
- **ci**: prefer Ninja on Windows and keep vcpkg toolchain integration
- ci compilation
- updated CODEOWNERS
- merge conflict markers in readme
- remove gettext from dockerfile and readme
- remove legacy gettext/msgfmt mentions
- **docker**: added missing dependencies for linux/amd64
- add possibly missing dependencies and do not copy local vcpkg
- prioritize python over python3 in windows build script
- adidng optional includ in SceneObject
- remove legacy gettext/msgfmt mentions
- **docker**: added missing dependencies for linux/amd64
- add possibly missing dependencies and do not copy local vcpkg
- update vcpkg version in release workflow
- add powershell call operator before python version check in build script
- install ninja in windows for ci
- allow windows to use ninja and make
- infer version from scm in packaging scripts
- enforce python3.10 to build
- raylib renderer black screen
- prioritize python over python3 in windows build script
- adidng optional includ in SceneObject
- adding content read permissions
- blocking all the pr with main as tagrget that doesnt come from hotifx/ or dev
- invalid indentation in the close-invalid-pr
- invalid indentation in the close-invalid-pr
- closing pr comments now include hotifx/
- adding back the tag in the pr workflow
- adding a check in the Dissector to avoid generating infinite cut planes
- removing double cstdint include
- using uint8_t when allowed for enum classes
- putting the CTrl + D shortcut back to normal
- adding cstdlib for SPLINTER3D_POSIX

### Refactor

- replace .cz.toml by .cz.yaml

## v0.0.1-rc (2026-03-10)

### Feat

- sign macos bundle
- **ci**: reusable build worklfow
- **macos**: created package (and fixed pre-commit)
- remove ci trigger on feat/ branch
- add trigger
- add a package-windows.ps1 for cleaner view of code
- remove the feat/ trigger for ci-linux
- remove ci-linux trigger on feat/ and test ci-windows with zip rename
- remove branch trigger on windows workflow and try linux workflow
- put the msgfmt on PATH on install
- clean the cmake
- remove the trigger on branch feat/create_exe
- added more log for debugging
- run workflow for testing purpose
- adding the cut button and the pannel use with the CTrl + X shortccup
- Adding first Cutting algo based on centroid fan
- using keybinds from a json file
- **Logger**: allow to log to a file and allow to enforce debug in normal build
- Adding event for the button panel keybinds
- clean duplication adn object deletion
- adding first inputmanager
- add possibility to choose exporting method and format
- add toBinarySTL
- Export all option
- Adding the rotation
- Adding an icon for transform
- renaming move into transform
- adding object movement
- adding multiple export option
- add basic export of selected object
- Scaling panel is now linked ot the current sleected object
- Checking objects hit
- Adding the import functionnality and mutliple object handling
- remove useless define and change namespace on setLanguage's call
- Making the scalepanel prettier
- linking scale with object
- adding button builder and documentation on the object objserver link
- adding layers logic
- put all base md files in .github folder
- cutting gui logic into parts
- adding camera yaw distance and pitch in the impl
- adding a custom camera to look like blender
- merging main into feat/gui
- add release.sh
- add .cz.toml to project
- added -g (fclean and debug build) to build.sh
- format files in pre-commit
- base logger
- using an Object3D structure to render and store matrix
- adding OMesh to RMesh observer pattern
- add rescaling menu popup
- add responsive model size and window
- added 5 buttons placeholders buttons
- added macos vcpkg include path in cpp properties
- adding the ascii and binary stl handling
- second merge from base rendrer
- adding the Objects3D namespace
- first merge from base rendrer
- adding the V2 of the 3d raylib renderer
- base files

### Fix

- run staple on main exec
- run staple on main exec
- run staple on binary instead of dmg
- codesign dmg to allow spctl to sign
- **ci**: renamed github token -> token to avoid collision
- removed useless casts in TransformPannelState.cpp
- **ci**: release name
- popups height now adapt to button contained
- **ci**: start ci either on tag push or manually
- merge conflicts
- patch build.ps1 to call package-windows.ps1 with explicit named parameters
- add an exit 0 to get right exit code
- change ubuntu runs-on version
- splinter3D exec not working on Linux
- package name searching
- try to find the exec on build/ and after on root
- old include error
- rollback as before on install+build on windows workflow
- change API.hpp for SPLINTER3D_API
- NFD dependencie issue
- compilation error on Linux
- merge conflicts
- workflows on workflows/ root
- clean archi and changing path
- release workflows and clean archi
- remove warnings of deprecated method
- msgfmt not found
- reset the build.ps1 to commit 9b2e1f1
- exe that crashed and build vcpkg error
- github_token put in env and .exe built on .zip
- remove cache test, and add permissions
- adding permissions for release
- checking for msgfmt on install
- clean the cmakes and choco dependencies
- build.ps1 script
- install.ps1
- the warning at the end of install.ps1
- Joion-Path error
- path on packages
- trying to locate gettext packages
- try to fix the thing
-  workflow now verifies vcpkg.exe
- now using curl
- parser error
- adding fallback on vcpkg bootstrap fail
- the here-string interpolation by creating a literal CMake ENV token and inserting it via a PowerShell variable
- gettext not detected and speed of workflow
- error on log print
- now writes a CMAKE_PROGRAM_PATH entry in the generated init file pointing to the build-specific vcpkg_installed/.../tools/gettext/bin
- vcpkg_init.cmake is now correctly multiline
- try to change the encoding
- vcpkg_init.cmake is now written with ASCII + CRLF
- remove too much
- remove warning from --manifest and try to write the init cache file
- trying to fix this error
- trying to fix this error
- trying to fix this error
- trying to fix this error
- trying to fix this error
- new way to build vcpkg
- lets clone the vcpkg repo
- remove error
- copying from r-type build.ps1
- adding dependencies correctly
- Mesh Dissector logic in order to return planes cuts to fit the boundaries
- include error in logger and logging eventbus
- fusion feature and selection buttons
- rotation panel name
- rotation panel name
- color of dropping menu and click both button at the same time
- using radiant for rotation and degrees for input
- scale now support floats, and floatvaluebox support minus
- **macos**: fixed macos compilation with clang
- removing all the color logic in objects3D::Objects3D
- singleton firend fiels in scalepanel, scene and eventbus
- added SPLINTER3D_API to logger functions for better cross platform compatibility
- rendering the 3d object on the world layer
- moved language compilation in new cmake and moved locale debug logs to logger
- merge conflict
- merge conflict
- added codeowners file
- eof compliance for all files
- fixed macos compilation by adding diagnostic ignore to raygui include
- removed old splinter folder
- moved raylibToLogger into RaylibRenderer
- task.json version number
- adding ensureCCW in the RaylibRenderer header file
- changed eol in some hpp to lf

### Refactor

- **Logger**: renamed oss to os in internal functions
- Objects3D logic in cpp file
- **Scene**: made Scene a class and moved its methods to a cpp file
- **gui**: moved all non-trivial functions out of hpp files
- **Logger.hpp**: logger functions factory
- renamed 7-Language.cmake to 7-Locale.cmake
- **Locale.hpp**: moved function implementations to Locale.cpp
- changed all splinter references to splinter3d

### Perf

- improved Utils exception spec

# VCPKG Build Report

The purpose of this report is to document my research on the differents problems of the vcpkg build:
- The build is time consuming: 50min and more depending of differents cases
- The vcpkg folder is taking a lot of space

1/
With the current vcpkg.json config and build.py, on my Windows, I got:
- Time: **~45min**
- vcpkg folder size: **21Go**
Without glfw3 and nativefiledialog-extended on vcpkg.json (that seems unused), on my Windows, I got:
- Time: **~45min**
- vcpkg folder size: **20Go**
===> these two libraries are not that big

2/
After a further look I saw that OCCT get compiled 2x, everytime. One time for a Debug mode, and another for a Release one.
I tried to configure vcpkg and the build flow to get only one mode compiled (more precisely, the triplet used).
I added a debug-only mode and a release-only mode to replace the default triplet.
Result of using release-only mode on my Windows:
- Time: **~28min**
- vcpkg folder size: **11Go**
===> this change clearely did something
The release-only specify clearely to vcpkg to only get one "mode", so it does not compile 2x the libraries.
I also suppose that WxWidgets was compiling 2x with the default triplet mode.

/!\ After testing, it seems to have no debug-only mode /!\

3/
I will look further, some OCCT libraries are not usefull for our project,
maybe is it possible to remove them from the vcpkg.json config, and so reduce the build time and the vcpkg folder size.

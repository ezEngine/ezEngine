How to set up Qt {#HowToSetupQt}
================

Using pre-compiled binaries
---------------------------

For this you need access to the Dropbox folder 'ezShare'.
If you don't have that, check out 'Compile Qt yourself' below.

1. Download http://download.qt-project.org/official_releases/qt/5.0/5.0.2/single/qt-everywhere-opensource-src-5.0.2.zip
2. Extract content to some folder (e.g. 'C:\\Qt\\5.0.2\\'), omitting the folder that is in the root of the zip
3. Extract 'vs100x64.7z' and / or 'vs100x86.7z' from 'Dropbox\\ezShare\\Qt' into the qt directory so the folders reside next to 'qtbase' etc.


Setup for CMake
---------------

1. Set your env var 'QTDIR' to '[installDir]\\vs100x64' or you may need to set 'EZ_QT_DIR' manually in the CMake GUI (during 'Configure').
2. Add '[installDir]\\vs100x64\bin' to your path so MOC etc. are found.
3. Enable EZ_ENABLE_QT_SUPPORT in order to build qt projects and re-configure.
4. Qt enabled projects should now be added to your solution.

Compile Qt yourself
-------------------

1. Download http://download.qt-project.org/official_releases/qt/5.0/5.0.2/single/qt-everywhere-opensource-src-5.0.2.zip
2. Extract content to some folder (e.g. 'C:\\Qt\\5.0.2\\'), omitting the folder that is in the root of the zip
3. Open Visual Studio (x64 Win64) Command Prompt (or x86 version, depending on what you are building)
4. cd to your qt root
5. Run the following commands (replace x64 with x86 if you are building 32bit)

        configure -confirm-license -opensource -opengl desktop -nomake tests -nomake examples -nomake demos -skip qtxmlpatterns -skip qtmultimedia -skip qtactiveqt -skip qtwebkit -skip qtwebkit-examples-and-demos -skip qtgraphicaleffects -skip qtquick1 -skip qttranslations -skip qtdeclarative -skip qtjsbackend -skip qtdoc -skip qttools -prefix %CD%\\vs100x64 -platform win32-msvc2010 -no-icu -mp
6. Run 'nmake'
7. Run 'nmake install'
8. Run 'cd qttools'
9. Run '..\\vs100x64\\bin\\qmake.exe -makefile -r'
10. Run 'cd src\\designer'
11. Run 'nmake'
12. Run 'nmake install'
13. Create '\\vs100x64\\bin\\qt.conf'
14. Insert

        [Paths]
        Prefix=..
        Documentation=doc
        Headers=include
        Libraries=lib
        Binaries=bin
        Plugins=plugins
        Imports=imports
        Data=
        Translations=
        Settings=
        Examples=
        Demos=
15. You are done!

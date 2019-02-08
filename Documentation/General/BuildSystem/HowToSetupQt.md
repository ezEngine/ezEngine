How to set up Qt {#HowToSetupQt}
================


Getting Qt
---------------------------

Get binaries for Qt from https://www.qt.io/ or compile it yourself. You will need support for
* QtCore
* QtGui
* QtWidgets
* QtNetwork
* QtWinExtras

The latest version of Qt used by ezEngine is **5.10.1**


Configuring the Build
---------------

By default ezEngine is built without Qt support. This will exclude the entire editor and all tools. To enable building these, you need to configure CMake to find the Qt include files and libraries:

1. Open the CMake GUI and enable **EZ\_ENABLE\_QT\_SUPPORT**
2. After running 'Configure' once, the CMake variable **EZ\_QT\_DIR** will show up
3. Point this variable to the top level folder of your Qt installation. Ie. the one containing the 'include' and 'lib' folders.
4. Run 'Configure' and 'Generate' again. The Visual Studio solutions will now contain the tools projects.


#cmake

#
# Copy this file into the folder of an application to add it to the applications solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

# Pass a boolean expression into this macro to build this project only if the expression evaluates to true.
#BUILD_CONDITIONAL(BUILDSYSTEM_PLATFORM_WINDOWS AND NOT BUILDSYSTEM_PLATFORM_64BIT)

BUILD_APPLICATION_PROJECT()

# Add all the preprocessor definitions that are needed to configure this application
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Add additional include directories, if required
#include_directories ("Stuff/include")

# Add additional library directories, if required
#link_directories ("Stuff/lib")

# add all the library references that the application needs here
SET (LINK_LIBRARIES
  Foundation # All projects that are created through cmake can be easily referenced by name
)

# Set this to true, if your application is a real win32 app (not a console app)
#SET (IS_WIN32_APPLICATION true)

# Enables the use of Qt in this project and sets up the required modules.
#QT_MODULES(Core Gui Widgets)

# Call this in case this project does not function without Qt.
#BUILD_REQUIRES_QT()

# Set all headers, that shall be MOC'ed that do not end with moc.h and thus will be MOC'ed automatically.
#SET (EZ_MOC_HEADERS
#  mainwindow.h
#)

# execute the "application"-template
include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Application.txt)

# Adds this project to 'Tests.txt', notifying the build server to run this as a test.
# The first parameter sets whether this tests needs direct hardware access to the GPU.
#ADD_TEST(OFF)
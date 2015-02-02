#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

if (POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW)
endif()

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Add all the libraries that this project needs to link against here
SET (LINK_LIBRARIES
  Foundation      # Example: Another library build with cmake
  CoreUtils
)


# Enables the use of Qt in this library and sets up the required modules.
QT_MODULES(Core Gui Widgets)

# Set all headers, that shall be MOC'ed that do not end with moc.h and thus will be MOC'ed automatically.
SET (EZ_MOC_HEADERS
  Framework/Qt/qtTestFramework.h
  Framework/Qt/qtTestGUI.h
  Framework/Qt/qtTestModel.h
  Framework/Qt/qtTestDelegate.h
  Framework/Qt/qtLogMessageDock.h
)

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Library.txt)

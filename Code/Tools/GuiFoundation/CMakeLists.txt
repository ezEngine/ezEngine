#cmake

#
# Copy this file into the folder of an experimental library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Do not use this file for 'Engine' libraries ***
#
# *** Modify this file as required ***
#

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Add all the libraries that this project needs to link against here
SET (LINK_LIBRARIES
  Foundation      # Example: Another library build with cmake
  ToolsFoundation
)

# Enables the use of Qt in this library and sets up the required modules.
#QT_MODULES(Core Gui Widgets)

# Call this in case this library does not function without Qt.
#BUILD_REQUIRES_QT()

# Set all headers, that shall be MOC'ed that do not end with moc.h and thus will be MOC'ed automatically.
#SET (EZ_MOC_HEADERS
#  testWidget.h
#)

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Library.txt)


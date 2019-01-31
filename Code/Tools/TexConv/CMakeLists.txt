#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

# Pass a boolean expression into this macro to build this project only if the expression evaluates to true.
BUILD_CONDITIONAL(BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Add all the libraries that this project needs to link against here
SET (LINK_LIBRARIES
  Foundation
  Core
  Utilities
  TexConvLib
)

# Visual Studio 2017 issues warnings because DirectXTex uses deprecated functions.
# This may be fixed in later versions of DirectXTex.
if (BUILDSYSTEM_COMPILER_MSVC)
  set (COMPILER_SETTINGS_ALL "${COMPILER_SETTINGS_ALL} /wd\"4996\"")
endif ()

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Application.txt)

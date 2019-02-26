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

BUILD_CONDITIONAL(BUILDSYSTEM_PLATFORM_WINDOWS)

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

BUILD_REQUIRES_DX11()

# Add all the libraries that this project needs to link against here
SET (LINK_LIBRARIES
  Foundation
  Core
  System
  GameEngine
  ThirdParty
  RendererFoundation
  RendererCore
  Utilities
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")

#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

BUILD_CONDITIONAL(BUILDSYSTEM_PLATFORM_WINDOWS)

BUILD_REQUIRES_DX11()

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Add all the libraries that this project needs to link against here
SET (LINK_LIBRARIES
  ThirdParty
  Foundation
  TestFramework
  Core
  Texture
  RendererCore
  RendererFoundation
  RendererDX11
  System
  GameEngine
)

SET (ADDITIONAL_DEPENDENCIES
  ShaderCompilerHLSL
)

if (BUILDSYSTEM_PLATFORM_WINDOWS_UWP)
  LIST(APPEND ADDITIONAL_DEPENDENCIES ShaderCompilerHLSL)
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

if (BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
  # Adds this project to 'Tests.txt', notifying the build server to run this as a test.
  # The first parameter sets whether this tests needs direct hardware access to the GPU.
  ADD_TEST(ON)
endif()

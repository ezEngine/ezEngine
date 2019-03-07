#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

BUILD_REQUIRES_DX11()

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
)

SET (ADDITIONAL_DEPENDENCIES
  ShaderCompilerHLSL
)

if (BUILDSYSTEM_PLATFORM_WINDOWS_UWP)
  LIST(APPEND ADDITIONAL_DEPENDENCIES ShaderCompilerHLSL)
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

# Adds this project to 'Tests.txt', notifying the build server to run this as a test.
# The first parameter sets whether this tests needs direct hardware access to the GPU.
ADD_TEST(ON)

#cmake

SET (LINK_LIBRARIES
  Foundation
  Core
  System
  ThirdParty
  RendererFoundation
  RendererCore
  RendererDX11
  Texture
)

if (BUILDSYSTEM_PLATFORM_WINDOWS_UWP)
  LIST(APPEND LINK_LIBRARIES FileservePlugin)
endif()

if (BUILDSYSTEM_PLATFORM_WINDOWS_UWP AND EZ_ENABLE_MIXEDREALITY_SUPPORT)
  LIST(APPEND LINK_LIBRARIES WindowsMixedReality)
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

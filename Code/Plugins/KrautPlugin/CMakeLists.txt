#cmake

if (NOT EZ_BUILD_KRAUT)
	return()
endif()

SET (LINK_LIBRARIES
  Foundation
  Core
  GameEngine
  RendererFoundation
  RendererCore
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

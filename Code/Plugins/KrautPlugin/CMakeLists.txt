#cmake

set (EZ_BUILD_KRAUT ON CACHE BOOL "Whether support for Kraut should be added")
mark_as_advanced(FORCE EZ_BUILD_KRAUT) # this can typically stay on

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

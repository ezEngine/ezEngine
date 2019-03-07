#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
	return()
endif()

SET (LINK_LIBRARIES
  Foundation
  Core
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

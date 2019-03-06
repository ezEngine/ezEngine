#cmake

set (EZ_BUILD_SAMPLES ON CACHE BOOL "Whether sample projects should be added to the solution")

if (NOT EZ_BUILD_SAMPLES)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_GlobSubDirs.txt")
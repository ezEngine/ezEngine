#cmake

set (EZ_BUILD_UNITTESTS ON CACHE BOOL "Whether unit-test projects should be added to the solution")

if (NOT EZ_BUILD_UNITTESTS)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_GlobSubDirs.txt")
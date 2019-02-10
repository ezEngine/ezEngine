#cmake

set (EZ_BUILD_UNITTESTS ON CACHE BOOL "Whether unit-test projects should be added to the solution")

if (NOT EZ_BUILD_UNITTESTS)
	return()
endif()

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_GlobUnitTests.txt)
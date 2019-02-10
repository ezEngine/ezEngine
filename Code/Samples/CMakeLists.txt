#cmake

set (EZ_BUILD_SAMPLES ON CACHE BOOL "Whether sample projects should be added to the solution")

if (NOT EZ_BUILD_SAMPLES)
	return()
endif()

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_GlobSamples.txt)
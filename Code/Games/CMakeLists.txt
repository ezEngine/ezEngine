#cmake

set (EZ_BUILD_GAMES OFF CACHE BOOL "Whether game projects should be added to the solution")

if (NOT EZ_BUILD_GAMES)
	return()
endif()

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_GlobGames.txt)
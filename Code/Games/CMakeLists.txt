#cmake

set (EZ_BUILD_GAMES OFF CACHE BOOL "Whether game projects should be added to the solution")

if (NOT EZ_BUILD_GAMES)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_GlobGames.txt")
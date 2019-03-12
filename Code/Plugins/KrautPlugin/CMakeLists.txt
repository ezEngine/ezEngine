#cmake

set (EZ_BUILD_KRAUT ON CACHE BOOL "Whether support for Kraut should be added")
mark_as_advanced(FORCE EZ_BUILD_KRAUT) # this can typically stay on

if (NOT EZ_BUILD_KRAUT)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

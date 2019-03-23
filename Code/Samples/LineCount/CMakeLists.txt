ez_all_vars()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Core
)

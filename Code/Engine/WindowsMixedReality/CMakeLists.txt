#cmake

ez_all_vars()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS_UWP)
	return()
endif()

if (NOT EZ_ENABLE_MIXEDREALITY_SUPPORT)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  RendererCore
)
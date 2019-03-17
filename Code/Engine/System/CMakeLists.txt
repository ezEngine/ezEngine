#cmake

ez_all_vars()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

if (EZ_CMAKE_PLATFORM_WINDOWS_UWP)

  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Xinputuap
  )

elseif (EZ_CMAKE_PLATFORM_WINDOWS)

  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    XINPUT9_1_0
  )

endif()

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Core
)

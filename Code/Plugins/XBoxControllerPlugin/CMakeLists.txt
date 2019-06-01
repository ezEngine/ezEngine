ez_cmake_init()

ez_requires(EZ_CMAKE_PLATFORM_WINDOWS)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Core
)

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

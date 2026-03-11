ez_cmake_init()



# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  RendererCore
  ozz
  Utilities
)

if (EZ_3RDPARTY_IMGUI_SUPPORT)
  target_link_libraries(${PROJECT_NAME} PUBLIC Imgui)
endif()

# XBOX controller support
if (EZ_CMAKE_PLATFORM_WINDOWS)

  if (EZ_3RDPARTY_GLFW_SUPPORT)
    # When using glfw, glfw will handle xbox controller input, and this plugin is not needed.
  else()

    if (EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)

      target_link_libraries(${PROJECT_NAME}
        PRIVATE
        XINPUT9_1_0
      )

    endif()

  endif()

endif()

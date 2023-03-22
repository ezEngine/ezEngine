ez_cmake_init()



ez_requires(EZ_CMAKE_PLATFORM_WINDOWS)

# When using glfw, glfw will handle xbox controller input, and this plugin is not needed.
if(EZ_3RDPARTY_GLFW_SUPPORT)
  return()
endif()


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

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/XBoxController.ezPluginBundle" $<TARGET_FILE_DIR:${PROJECT_NAME}>
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	  )

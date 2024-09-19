ez_cmake_init()

ez_requires_renderer()
ez_requires_desktop()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

ez_link_target_dx11(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/SampleGame.ezPluginBundle" $<TARGET_FILE_DIR:${PROJECT_NAME}>
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	  )

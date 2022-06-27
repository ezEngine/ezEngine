ez_cmake_init()

ez_requires_editor()

ez_requires(EZ_3RDPARTY_DUKTAPE_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  GameEngine
  EditorPluginAssets
  TypeScriptPlugin
)

add_dependencies(${PROJECT_NAME}
  TypeScriptPlugin
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/TypeScript.ezPluginBundle" $<TARGET_FILE_DIR:${PROJECT_NAME}>
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	  )

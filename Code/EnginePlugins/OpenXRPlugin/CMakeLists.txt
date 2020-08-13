ez_cmake_init()

ez_requires_openxr()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_OPENXR_SUPPORT)
if (NOT EZ_OPENXR_PREVIEW_DIR STREQUAL "")
	target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_OPENXR_PREVIEW_SUPPORT)
endif()
    
ez_link_target_openxr(${PROJECT_NAME})

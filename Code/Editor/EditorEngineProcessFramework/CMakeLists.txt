ez_cmake_init()

ez_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE

)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ToolsFoundation
  GameEngine
  Utilities
)

if(EZ_3RDPARTY_GLFW_SUPPORT AND EZ_CMAKE_PLATFORM_LINUX)
  target_link_libraries(${PROJECT_NAME} PRIVATE xcb)
endif()

# On linux we need the renderer for the in editor render loop + shared texture setup
if(EZ_CMAKE_PLATFORM_LINUX)
  target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENGINE_PROCESS_SHARED_TEXTURE)
endif()
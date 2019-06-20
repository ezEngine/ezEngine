ez_cmake_init()

ez_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE

  Foundation
  Core
  Texture
  RendererFoundation
  RendererCore
  System
  GameEngine
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ToolsFoundation
)

# if (EZ_CMAKE_PLATFORM_WINDOWS_UWP AND EZ_ENABLE_MIXEDREALITY_SUPPORT)
# 
#   target_link_libraries(${PROJECT_NAME}
#     PRIVATE
#     WindowsMixedReality
#   )
# 
# endif()

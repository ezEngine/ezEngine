ez_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)
# FMOD is in a sub-module, when it isn't checked out ez_requires_fmod() doesn't exist, so early out here when it's not needed anyway
ez_apply_build_filter(${PROJECT_NAME})

ez_requires_editor()

ez_requires_fmod()

ez_create_target(LIBRARY ${PROJECT_NAME})

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  GameEngine
  FmodPlugin
)

ez_link_target_fmod(${PROJECT_NAME})



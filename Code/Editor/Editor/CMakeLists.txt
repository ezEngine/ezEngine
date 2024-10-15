ez_cmake_init()

ez_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

ez_make_windowapp(${PROJECT_NAME})

ez_add_output_ez_prefix(${PROJECT_NAME})

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network Svg)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  EditorEngineProcessFramework
)

add_dependencies(${PROJECT_NAME}
  EditorProcessor
  EditorEngineProcess
  Player
)

# For SetProcessDpiAwareness
if (EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Shcore.lib
  )
endif()

set(EDITOR_DATA_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../../../Data/Tools/ezEditor")
file(GLOB_RECURSE EDITOR_DATA "${CMAKE_CURRENT_SOURCE_DIR}/../../../Data/Tools/ezEditor/*")

source_group(TREE ${EDITOR_DATA_FOLDER} PREFIX "Editor Data" FILES ${EDITOR_DATA})
target_sources(${PROJECT_NAME} PRIVATE "${EDITOR_DATA}")

set_source_files_properties(${EDITOR_DATA} PROPERTIES HEADER_FILE_ONLY TRUE)

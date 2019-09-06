ez_cmake_init()

ez_requires_editor()

ez_requires_windows()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  TestFramework
  EditorFramework
  EditorEngineProcessFramework
)

add_dependencies(${PROJECT_NAME}
  EditorProcessor
  EditorEngineProcess
  EditorPluginScene
  EditorPluginAssets
  EditorPluginParticle
  EditorPluginProcGen
  ParticlePlugin
  TexConv2
  Player
  InspectorPlugin
  ShaderCompiler
)

if (TARGET EditorPluginFileserve)
  add_dependencies(${PROJECT_NAME} EditorPluginFileserve)
endif()

if (TARGET FmodPlugin)
  add_dependencies(${PROJECT_NAME} EditorPluginFmod FmodPlugin)
endif()

if (TARGET PhysXPlugin)
  add_dependencies(${PROJECT_NAME} EditorPluginPhysX PhysXPlugin)
endif()

if (TARGET KrautPlugin)
  add_dependencies(${PROJECT_NAME} EditorPluginKraut KrautPlugin)
endif()

if (TARGET RecastPlugin)
  add_dependencies(${PROJECT_NAME} RecastPlugin)
endif()

ez_ci_add_test(${PROJECT_NAME} NEEDS_HW_ACCESS)

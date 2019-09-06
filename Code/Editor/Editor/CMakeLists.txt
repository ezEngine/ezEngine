ez_cmake_init()

ez_requires_editor()

ez_requires_windows()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

ez_make_winmain_executable(${PROJECT_NAME})

ez_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
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

if (TARGET EditorPluginFmod)
	add_dependencies(${PROJECT_NAME} EditorPluginFmod)
endif()

if (TARGET PhysXPlugin)
	add_dependencies(${PROJECT_NAME} EditorPluginPhysX PhysXPlugin)
endif()

if (TARGET KrautPlugin)
	add_dependencies(${PROJECT_NAME} EditorPluginKraut KrautPlugin)
endif()

if (TARGET SampleGamePlugin)
	add_dependencies(${PROJECT_NAME} SampleGamePlugin)
endif()

if (TARGET RtsGamePlugin)
	add_dependencies(${PROJECT_NAME} RtsGamePlugin)
endif()

if (TARGET EditorPluginRecast)
	add_dependencies(${PROJECT_NAME} EditorPluginRecast)
endif()

if (TARGET RenderDocPlugin)
	add_dependencies(${PROJECT_NAME} RenderDocPlugin)
endif()

# set all external projects as runtime dependencies of this application
get_property(EXTERNAL_PROJECTS GLOBAL PROPERTY "EXTERNAL_PROJECTS")
if(EXTERNAL_PROJECTS)
	message(STATUS "External Projects: ${EXTERNAL_PROJECTS}")
	add_dependencies(${PROJECT_NAME} ${EXTERNAL_PROJECTS})
endif()



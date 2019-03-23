ez_cmake_init()

ez_requires_qt()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

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
  EditorPluginFileserve
  EditorPluginProceduralPlacement
  ParticlePlugin
  TexConv2
  Player
  InspectorPlugin
  ShaderCompiler
  RecastPlugin
  RenderDocPlugin
)

if (EZ_BUILD_FMOD)
	add_dependencies(${PROJECT_NAME} EditorPluginFmod FmodPlugin)
endif()

if (EZ_BUILD_PHYSX)
	add_dependencies(${PROJECT_NAME} EditorPluginPhysX PhysXPlugin)
endif()

if (EZ_BUILD_KRAUT)
	add_dependencies(${PROJECT_NAME} EditorPluginKraut KrautPlugin)
endif()

if (EZ_BUILD_GAMES)
	add_dependencies(${PROJECT_NAME} SampleGamePlugin)
endif()

if (EZ_BUILD_SAMPLES)
	add_dependencies(${PROJECT_NAME} RtsGamePlugin)
endif()

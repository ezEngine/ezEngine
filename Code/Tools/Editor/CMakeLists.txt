#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

# Set this to true, if your application is a real win32 app (not a console app)
SET (IS_WIN32_APPLICATION true)

SET (ADDITIONAL_DEPENDENCIES
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
	SET (ADDITIONAL_DEPENDENCIES ${ADDITIONAL_DEPENDENCIES} EditorPluginFmod FmodPlugin)
endif()

if (EZ_BUILD_PHYSX)
	SET (ADDITIONAL_DEPENDENCIES ${ADDITIONAL_DEPENDENCIES} EditorPluginPhysX PhysXPlugin)
endif()

if (EZ_BUILD_KRAUT)
	SET (ADDITIONAL_DEPENDENCIES ${ADDITIONAL_DEPENDENCIES} EditorPluginKraut KrautPlugin)
endif()

if (EZ_BUILD_GAMES)
	SET (ADDITIONAL_DEPENDENCIES ${ADDITIONAL_DEPENDENCIES}
    SampleGamePlugin
    )
endif()

if (EZ_BUILD_SAMPLES)
	SET (ADDITIONAL_DEPENDENCIES ${ADDITIONAL_DEPENDENCIES}
    # RtsGamePlugin
    )
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

ez_link_target_qt(${PROJECT_NAME} Core Gui Widgets Network)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorFramework
  EditorEngineProcessFramework
)

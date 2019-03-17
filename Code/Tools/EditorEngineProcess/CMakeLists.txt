#cmake

ez_all_vars()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

# Set this to true, if your application is a real win32 app (not a console app)
SET (IS_WIN32_APPLICATION true)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

ez_link_target_dx11(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  RendererDX11
  GameEngine
  EditorEngineProcessFramework
)

# For plugins to work in UWP builds, they must be direct link dependencies, otherwise they are not packaged with the app.
if (EZ_CMAKE_PLATFORM_WINDOWS_UWP)

  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    InspectorPlugin
  )

  if (EZ_BUILD_FMOD)

    target_link_libraries(${PROJECT_NAME}
      PRIVATE
      FmodPlugin
    )

  endif()

  if (EZ_BUILD_PHYSX)

    target_link_libraries(${PROJECT_NAME}
      PRIVATE
      PhysXPlugin
      PhysXCooking
    )

  endif()
endif()

add_dependencies(${PROJECT_NAME}
  ShaderCompilerHLSL
  EnginePluginScene
  EnginePluginAssets
  EnginePluginParticle
  RecastPlugin
  InspectorPlugin
)

ez_cmake_init()

ez_requires_editor()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

ez_make_winmain_executable(${PROJECT_NAME})

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

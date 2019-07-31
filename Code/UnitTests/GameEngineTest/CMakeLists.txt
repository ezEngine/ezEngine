ez_cmake_init()

ez_requires_d3d()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  GameEngine
  RendererDX11
)

if (EZ_CMAKE_PLATFORM_WINDOWS_UWP)
  # Due to app sandboxing we need to explcitly name required plugins for UWP.
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
    KrautPlugin
    ParticlePlugin
    InspectorPlugin
  )

  if (EZ_BUILD_FMOD)
    target_link_libraries(${PROJECT_NAME} PUBLIC FmodPlugin)
  endif()

endif()


ez_link_target_dx11(${PROJECT_NAME})

ez_ci_add_test(${PROJECT_NAME} NEEDS_HW_ACCESS)

add_dependencies(${PROJECT_NAME}
  ShaderCompilerHLSL
)


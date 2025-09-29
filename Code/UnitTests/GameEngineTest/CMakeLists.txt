ez_cmake_init()

ez_requires_renderer()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  GameEngine
  RendererCore
  Utilities
  ParticlePlugin
  VisualScriptPlugin  
)

if (EZ_3RDPARTY_ANGELSCRIPT_SUPPORT)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
    AngelScriptPlugin
  )
endif()

if (EZ_BUILD_RMLUI AND (EZ_CMAKE_PLATFORM_WINDOWS OR EZ_CMAKE_PLATFORM_LINUX))
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
    RmlUiPlugin
  )
endif()

ez_add_renderers(${PROJECT_NAME})

ez_ci_add_test(${PROJECT_NAME} NEEDS_HW_ACCESS)



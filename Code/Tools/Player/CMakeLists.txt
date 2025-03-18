ez_cmake_init()

ez_requires_renderer()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})
ez_make_windowapp(${PROJECT_NAME})

ez_add_output_ez_prefix(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  GameEngine
)

if (EZ_CMAKE_PLATFORM_WINDOWS_UWP)
  # Due to app sandboxing we need to explicitly name required plugins for UWP.
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
    ParticlePlugin
    InspectorPlugin
  )

  if (EZ_BUILD_PHYSX)
    target_link_libraries(${PROJECT_NAME} PUBLIC PhysXPlugin)
  endif()

  if (EZ_BUILD_FMOD)
    target_link_libraries(${PROJECT_NAME} PUBLIC FmodPlugin)
  endif()

  if (EZ_BUILD_OPENXR)
    find_package(ezOpenXR REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenXRPlugin)
  endif()

endif()

ez_add_renderers(${PROJECT_NAME})

add_dependencies(${PROJECT_NAME}
  InspectorPlugin
)

ez_cmake_init()

ez_requires_windows()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

ez_make_winmain_executable(${PROJECT_NAME})

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
    TypeScriptPlugin
  )

  if (EZ_BUILD_FMOD)
    find_package(EzFmod REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC FmodPlugin)
  endif()

  if (EZ_BUILD_OPENXR)
    find_package(ezOpenXR REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenXRPlugin)
  endif()

endif()

add_dependencies(${PROJECT_NAME}
  ShaderCompilerHLSL
  InspectorPlugin
)

# set all external projects as runtime dependencies of this application
get_property(EXTERNAL_PROJECTS GLOBAL PROPERTY "EXTERNAL_PROJECTS")
if(EXTERNAL_PROJECTS)
	message(STATUS "External Projects: ${EXTERNAL_PROJECTS}")
	add_dependencies(${PROJECT_NAME} ${EXTERNAL_PROJECTS})
endif()

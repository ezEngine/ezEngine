ez_all_vars()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

if (NOT EZ_BUILD_PHYSX)
	return()
endif()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

ez_link_target_dx11(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorEngineProcessFramework
  GameEngine
  PhysXPlugin
  PhysXCooking
)

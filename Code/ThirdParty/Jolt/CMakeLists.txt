ez_cmake_init()

ez_requires(EZ_3RDPARTY_JOLT_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX)
if (EZ_COMPILE_ENGINE_AS_DLL)
	target_compile_definitions(${PROJECT_NAME} PUBLIC JPH_SHARED_LIBRARY)
	target_compile_definitions(${PROJECT_NAME} PRIVATE JPH_BUILD_SHARED_LIBRARY)
endif()

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_JOLT_SUPPORT JPH_DEBUG_RENDERER)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})


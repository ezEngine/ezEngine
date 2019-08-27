ez_cmake_init()

ez_requires(EZ_3RDPARTY_ZLIB_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_ZLIB_SUPPORT)

if (EZ_COMPILE_ENGINE_AS_DLL)

  target_compile_definitions(${PROJECT_NAME} PRIVATE ZLIB_DLL_EXPORT=1)
  target_compile_definitions(${PROJECT_NAME} PUBLIC ZLIB_DLL_IMPORT=1)

endif()

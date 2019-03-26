ez_cmake_init()

set (EZ_3RDPARTY_ZSTD_SUPPORT ON CACHE BOOL "Whether to add support for zstd.")
mark_as_advanced(FORCE EZ_3RDPARTY_ZSTD_SUPPORT)

ez_requires(EZ_3RDPARTY_ZSTD_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_ZSTD_SUPPORT)

target_include_directories(${PROJECT_NAME} PRIVATE common compress decompress)

if (EZ_COMPILE_ENGINE_AS_DLL)

  target_compile_definitions(${PROJECT_NAME} PRIVATE ZSTD_DLL_EXPORT=1)
  target_compile_definitions(${PROJECT_NAME} PUBLIC ZSTD_DLL_IMPORT=1)

endif()

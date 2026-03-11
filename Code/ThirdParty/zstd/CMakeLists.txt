ez_cmake_init()

ez_requires(EZ_3RDPARTY_ZSTD_SUPPORT)

# not sure why this is needed, but otherwise there's a warning for this project
set(CMAKE_MSVC_RUNTIME_LIBRARY "")

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_ZSTD_SUPPORT PRIVATE ZSTD_MULTITHREAD ZSTD_DISABLE_ASM)

target_include_directories(${PROJECT_NAME} PRIVATE common compress decompress)

if (EZ_COMPILE_ENGINE_AS_DLL AND EZ_CMAKE_PLATFORM_WINDOWS)

  target_compile_definitions(${PROJECT_NAME} PRIVATE ZSTD_DLL_EXPORT=1)
  target_compile_definitions(${PROJECT_NAME} PUBLIC ZSTD_DLL_IMPORT=1)

endif()

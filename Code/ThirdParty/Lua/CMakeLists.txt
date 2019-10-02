ez_cmake_init()

ez_requires(EZ_3RDPARTY_LUA_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_LUA_SUPPORT)

# Tell lua to use posix compliant APIs in case we are on a posix platform.
if(EZ_CMAKE_PLATFORM_POSIX)
  target_compile_definitions(${PROJECT_NAME} PRIVATE LUA_USE_POSIX)
endif()

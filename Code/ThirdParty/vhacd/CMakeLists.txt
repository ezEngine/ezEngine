ez_cmake_init()

ez_build_filter_foundation()

ez_requires(EZ_3RDPARTY_VHACD_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_VHACD_SUPPORT)

target_include_directories(${PROJECT_NAME} PRIVATE inc public)

if (EZ_COMPILE_ENGINE_AS_DLL)

  target_compile_definitions(${PROJECT_NAME} PRIVATE VHACD_BUILDING_LIB=1)
  target_compile_definitions(${PROJECT_NAME} PUBLIC VHACD_SHARED_LIBRARY=1)

endif()

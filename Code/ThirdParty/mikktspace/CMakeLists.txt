ez_cmake_init()



# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX)

if(EZ_CMAKE_PLATFORM_WINDOWS)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4456)
endif()

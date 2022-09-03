ez_cmake_init()

ez_requires(EZ_3RDPARTY_KRAUT_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(SHARED_LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX)

target_compile_definitions(${PROJECT_NAME} PUBLIC AE_COMPILE_ENGINE_AS_DLL)
target_compile_definitions(${PROJECT_NAME} PRIVATE BUILDSYSTEM_BUILDING_KRAUTGENERATOR_LIB BUILDSYSTEM_BUILDING_KRAUTFOUNDATION_LIB)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

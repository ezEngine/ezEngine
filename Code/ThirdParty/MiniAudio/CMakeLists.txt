ez_cmake_init()

ez_requires_miniaudio()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX NO_WARNINGS_AS_ERRORS)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_MINIAUDIO_SUPPORT MA_DLL)

target_compile_definitions(${PROJECT_NAME} PRIVATE MA_NO_FLAC MA_NO_ENCODING MA_NO_GENERATION MA_NO_CUSTOM MA_NO_WINMM)

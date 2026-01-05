#ez-version 2
ez_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME} NO_EZ_PREFIX NO_UNITY)
ez_make_windowapp(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

# include the EZ natvis file for a better debugging experience
get_property(EZ_ROOT_DIR GLOBAL PROPERTY EZ_ROOT)
target_sources(${PROJECT_NAME} PUBLIC "${EZ_ROOT_DIR}/Code/Engine/Foundation/ezEngine.natvis")

target_compile_definitions(${PROJECT_NAME} PRIVATE "GAME_PROJECT_FOLDER=${CMAKE_CURRENT_LIST_DIR}/../../")

add_dependencies(${PROJECT_NAME} CppProjectPlugin)

# Include optional extensions set up by the ezEditor
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Configs/CMakeEngineExtensions.txt")
	include("${CMAKE_CURRENT_SOURCE_DIR}/../Configs/CMakeEngineExtensions.txt")
endif()

# Include optional user extensions for custom target configuration
# This file can be used to add additional link libraries, includes, etc. without modifying this template
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CMakeExtensions.txt")
	include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeExtensions.txt")
endif()

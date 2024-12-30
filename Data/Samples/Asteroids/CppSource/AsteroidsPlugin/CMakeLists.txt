ez_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(SHARED_LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX NO_UNITY)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

# include the EZ natvis file for a better debugging experience
get_property(EZ_ROOT_DIR GLOBAL PROPERTY EZ_ROOT)
target_sources(${PROJECT_NAME} PUBLIC "${EZ_ROOT_DIR}/Code/Engine/Foundation/ezEngine.natvis")


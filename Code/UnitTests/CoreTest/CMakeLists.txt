ez_cmake_init()




# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

if(EZ_CMAKE_PLATFORM_ANDROID)
#TODO: Add actual packaging code. This is done in PRE_BUILD so that it happens before the
#apk gen steps that happen in POST_BUILD and which are already done via ez_create_target. 
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${EZ_ROOT}/Data/UnitTests/CoreTest/ ${CMAKE_CURRENT_BINARY_DIR}/package/Assets/Data/UnitTests/CoreTest/)
endif()  

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  Core
  Texture
)

ez_project_build_filter_index(${PROJECT_NAME} BUILD_FILTER_IDX)

if(TARGET TexConv)
  target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_TEXCONV_PRESENT)
  
  add_dependencies(${PROJECT_NAME}
    TexConv
  )
endif()

ez_ci_add_test(${PROJECT_NAME})

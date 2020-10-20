ez_cmake_init()

ez_build_filter_runtime()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  TestFramework
  Core
  Texture
)

ez_build_filter_get_index(BUILD_FILTER)

if(EZ_BUILD_TOOLS AND ${BUILD_FILTER} GREATER_EQUAL ${BUILD_FILTER_INDEX_EVERYTHING})
  target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_TEXCONV_PRESENT)
  add_dependencies(${PROJECT_NAME}
    TexConv
  )
endif()

ez_ci_add_test(${PROJECT_NAME})

ez_cmake_init()

ez_requires_renderer()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  RendererCore
)

ez_add_renderers(${PROJECT_NAME})


# ez_package_files(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/Output/ShaderCache/WGSL" "Output/ShaderCache/WGSL")
# ez_package_files(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/Data/Base" "Data/Base")
# ez_package_files(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/Data/Samples/MeshRenderSample" "Data/Samples/MeshRenderSample")

ez_cmake_init()

ez_requires_renderer()
ez_requires_desktop()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

# Package required data files for deployment
# This ensures the application has access to shaders and sample data
ez_package_files(${PROJECT_NAME} "${EZ_ROOT}/Data/Samples/ShaderExplorer" "Data/Samples/ShaderExplorer")
ez_package_files(${PROJECT_NAME} "${EZ_ROOT}/Data/Base/Shaders" "Data/Base/Shaders")

# Package shader cache if available (Vulkan shaders for Android/Web platforms)
if(EXISTS "${EZ_ROOT}/Output/ShaderCache/VULKAN")
    ez_package_files(${PROJECT_NAME} "${EZ_ROOT}/Output/ShaderCache/VULKAN" "Output/ShaderCache/VULKAN")
endif()

if(EZ_CMAKE_PLATFORM_ANDROID)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
        InspectorPlugin
    )
endif()

ez_add_renderers(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  RendererCore
)

add_dependencies(${PROJECT_NAME}
  InspectorPlugin
)

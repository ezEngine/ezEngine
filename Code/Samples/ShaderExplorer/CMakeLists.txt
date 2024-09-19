ez_cmake_init()

ez_requires_renderer()
ez_requires_desktop()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(APPLICATION ${PROJECT_NAME})

if(EZ_CMAKE_PLATFORM_ANDROID)
    #TODO: Add actual packaging code. This is done in PRE_BUILD so that it happens before the
    #apk gen steps that happen in POST_BUILD and which are already done via ez_create_target.
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${EZ_ROOT}/Output/ShaderCache/VULKAN/ ${CMAKE_CURRENT_BINARY_DIR}/package/Assets/Output/ShaderCache/VULKAN/)
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${EZ_ROOT}/Data/Samples/ShaderExplorer/ ${CMAKE_CURRENT_BINARY_DIR}/package/Assets/Data/Samples/ShaderExplorer/)
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${EZ_ROOT}/Data/Base/Shaders/ ${CMAKE_CURRENT_BINARY_DIR}/package/Assets/Data/Base/Shaders/)

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

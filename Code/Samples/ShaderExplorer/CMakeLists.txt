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

# Platform-specific packaging fallback
# TODO: Remove this once ez_platformhook_package_files is implemented for Android
if(EZ_CMAKE_PLATFORM_ANDROID)
    # Android-specific packaging using custom commands as fallback
    # This is done in PRE_BUILD so that it happens before the apk generation steps
    # that happen in POST_BUILD via ez_create_target
    set(ANDROID_PACKAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/package/Assets")
    
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ANDROID_PACKAGE_DIR}/Data/Samples/ShaderExplorer"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${EZ_ROOT}/Data/Samples/ShaderExplorer/" "${ANDROID_PACKAGE_DIR}/Data/Samples/ShaderExplorer/"
        COMMENT "Packaging ShaderExplorer sample data for Android")
        
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ANDROID_PACKAGE_DIR}/Data/Base/Shaders"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${EZ_ROOT}/Data/Base/Shaders/" "${ANDROID_PACKAGE_DIR}/Data/Base/Shaders/"
        COMMENT "Packaging base shaders for Android")
    
    # Only copy shader cache if it exists
    if(EXISTS "${EZ_ROOT}/Output/ShaderCache/VULKAN")
        add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${ANDROID_PACKAGE_DIR}/Output/ShaderCache/VULKAN"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${EZ_ROOT}/Output/ShaderCache/VULKAN/" "${ANDROID_PACKAGE_DIR}/Output/ShaderCache/VULKAN/"
            COMMENT "Packaging Vulkan shader cache for Android")
    endif()

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

ez_cmake_init()

ez_requires(EZ_BUILD_OPENXR)
ez_requires_desktop()

ez_fetch_openxr()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

if(EZ_BUILD_VULKAN AND EZ_CMAKE_PLATFORM_SUPPORTS_VULKAN)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Vulkan-Headers
  )
endif()

if (EZ_CMAKE_PLATFORM_LINUX)
    # We need to link against the Vulkan loader or SteamVR runtime will fail the call to xrGetVulkanGraphicsDevice2KHR with error:
    # IVRSystem::GetOutputDevice: Failed to get required Vulkan device function pointer (vkGetPhysicalDeviceProperties2KHR)
    # Thus, we can't use dynamic dispatch in Vulkan-hpp
    find_package(Vulkan REQUIRED)
    target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-nullability-completeness" "-Wno-enum-compare" "-Wno-switch")
    target_link_libraries(${PROJECT_NAME}
            PUBLIC
            Vulkan::Vulkan
    )
    target_compile_definitions(${PROJECT_NAME} PUBLIC EZ_VULKAN_DYNAMIC_DISPATCH=EZ_OFF )
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC EZ_VULKAN_DYNAMIC_DISPATCH=EZ_ON )
endif()

ez_link_target_openxr(${PROJECT_NAME})

ez_copy_plugin_bundle(${PROJECT_NAME} "OpenXR")

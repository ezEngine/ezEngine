ez_cmake_init()

ez_requires(EZ_BUILD_OPENXR)
ez_requires_assimp()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
  OpenXR

  PRIVATE
  assimp
)

# Link Vulkan support if available (for RendererVulkan integration)
if(TARGET RendererVulkan)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    RendererVulkan
    Vulkan-Headers
  )
  target_compile_definitions(${PROJECT_NAME} PRIVATE EZ_OPENXR_HAS_VULKAN_RENDERER=1)
else()
  target_compile_definitions(${PROJECT_NAME} PRIVATE EZ_OPENXR_HAS_VULKAN_RENDERER=0)
endif()

ez_link_target_openxr(${PROJECT_NAME})

ez_copy_plugin_bundle(${PROJECT_NAME} "OpenXR")

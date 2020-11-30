ez_cmake_init()

set (EZ_VULKAN_SDK_DIR "" CACHE PATH "Directory of the Vulkan SDK installation")

ez_build_filter_everything()
#ez_requires_vulkan()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC "${EZ_VULKAN_SDK_DIR}/include")

#ez_link_target_vulkan(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  RendererFoundation
  "${EZ_VULKAN_SDK_DIR}/lib/vulkan-1.lib"
)
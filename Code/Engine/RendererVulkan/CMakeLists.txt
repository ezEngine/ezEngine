ez_cmake_init()


ez_requires_vulkan()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

if (EZ_CMAKE_PLATFORM_LINUX)
	target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-nullability-completeness" "-Wno-enum-compare" "-Wno-switch")
endif()

ez_link_target_vulkan(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  RendererFoundation
)

if(EZ_3RDPARTY_GLFW_SUPPORT)

  target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

endif()

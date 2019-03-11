#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  RendererCore
  RendererDX11
  System
)

add_dependencies(${PROJECT_NAME}
  ShaderCompilerHLSL
  InspectorPlugin
)

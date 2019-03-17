#cmake

ez_all_vars()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)
	return()
endif()

# Set this to true, if your application is a real win32 app (not a console app)
SET (IS_WIN32_APPLICATION true)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
  RendererDX11
)

add_dependencies(${PROJECT_NAME}
  ShaderCompilerHLSL
)

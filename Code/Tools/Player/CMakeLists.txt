ez_cmake_init()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

ez_make_winmain_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  GameEngine
)

add_dependencies(${PROJECT_NAME}
  ShaderCompilerHLSL
  InspectorPlugin
)

if (EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)
  add_dependencies(${PROJECT_NAME} RenderDocPlugin)
endif()

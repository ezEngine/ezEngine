ez_all_vars()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

ez_make_winmain_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  GameEngine
)

add_dependencies(${PROJECT_NAME}
  ShaderCompilerHLSL
  InspectorPlugin
)

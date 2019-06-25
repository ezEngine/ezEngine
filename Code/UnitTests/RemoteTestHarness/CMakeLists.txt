ez_cmake_init()

ez_requires(EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)

ez_nuget_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target_cs(APPLICATION ${PROJECT_NAME})

add_dependencies(${PROJECT_NAME}
  Fileserve
)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DOTNET_REFERENCES
  "System"
  "System.Core"
  "System.Data"
  "${CMAKE_CURRENT_BINARY_DIR}/packages/CommandLineParser.2.5.0/lib/net461/CommandLine.dll"
  "${CMAKE_CURRENT_BINARY_DIR}/packages/Newtonsoft.Json.4.5.1/lib/net40/Newtonsoft.Json.dll"
)

add_custom_command(TARGET ${PROJECT_NAME}
	PRE_BUILD
	COMMAND ${NUGET} restore ${CMAKE_CURRENT_SOURCE_DIR}/packages.config -PackagesDirectory ${CMAKE_CURRENT_BINARY_DIR}/packages
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)


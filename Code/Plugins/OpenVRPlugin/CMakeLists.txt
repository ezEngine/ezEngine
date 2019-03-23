ez_cmake_init()

set (EZ_BUILD_OPENVR OFF CACHE BOOL "Whether support for OpenVR aka. SteamVR should be added")

if (NOT EZ_BUILD_OPENVR)
	return()
endif()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

AddOpenVR()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  GameEngine
)

CopyOpenVRDlls()

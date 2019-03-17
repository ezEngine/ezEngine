#cmake

ez_all_vars()

set (EZ_BUILD_OPENVR OFF CACHE BOOL "Whether support for OpenVR aka. SteamVR should be added")

if (NOT EZ_BUILD_OPENVR)
	return()
endif()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

AddOpenVR()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  GameEngine
)

CopyOpenVRDlls()

#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

SET (LINK_LIBRARIES 
  Foundation 
  RendererFoundation
  System
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

ez_link_target_dx11(${PROJECT_NAME})
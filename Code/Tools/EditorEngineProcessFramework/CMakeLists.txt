#cmake

ez_all_vars()

if (NOT EZ_CMAKE_PLATFORM_WINDOWS)
	return()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

target_link_libraries(${PROJECT_NAME}
  PRIVATE

  Foundation
  Core
  Texture
  RendererFoundation
  RendererCore
  System
  GameEngine
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  ToolsFoundation
)

# if (EZ_CMAKE_PLATFORM_WINDOWS_UWP AND EZ_ENABLE_MIXEDREALITY_SUPPORT)
# 
#   target_link_libraries(${PROJECT_NAME}
#     PRIVATE
#     WindowsMixedReality
#   )
# 
# endif()

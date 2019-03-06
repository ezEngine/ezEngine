#cmake

if (POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW)
endif()

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

if (NOT EZ_BUILD_FMOD)
	return()
endif()

QT_MODULES(Core Gui Widgets)

BUILD_REQUIRES_QT()

SET (LINK_LIBRARIES 
  Foundation
  EditorFramework
  ToolsFoundation
  Core
  GameEngine
  FmodPlugin
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")

find_package(EzFmod REQUIRED)

if (EZFMOD_FOUND)
  target_link_libraries(${PROJECT_NAME} ezFmod::Studio)

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezFmod::Studio> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ezFmod::LowLevel> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )

endif()

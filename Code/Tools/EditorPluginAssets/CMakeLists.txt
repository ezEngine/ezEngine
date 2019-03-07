#cmake

if (NOT BUILDSYSTEM_PLATFORM_WINDOWS)
	return()
endif()

QT_MODULES(Core Gui Widgets)

BUILD_REQUIRES_QT()

SET (LINK_LIBRARIES
  Foundation
  EditorFramework
  ToolsFoundation
  ThirdParty
  Core
  System
  RendererFoundation
  RendererDX11
  RendererCore
  GameEngine
  SharedPluginAssets
  ModelImporter
  Texture
)

SET (ADDITIONAL_DEPENDENCIES
  EnginePluginAssets
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Plugin.txt")

ez_link_target_dx11(${PROJECT_NAME})

find_package(EzAssImp REQUIRED)

if (EZASSIMP_FOUND)

  target_link_libraries(${PROJECT_NAME} EzAssImp::EzAssImp)

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:EzAssImp::EzAssImp> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )

endif()

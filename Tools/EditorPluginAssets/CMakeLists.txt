#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

BUILD_CONDITIONAL(BUILDSYSTEM_PLATFORM_WINDOWS)

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Enables the use of Qt in this project and sets up the required modules.
QT_MODULES(Core Gui Widgets)

# Call this in case this project does not function without Qt.
BUILD_REQUIRES_QT()

BUILD_REQUIRES_DX11()

# Add all the libraries that this project needs to link against here
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
)

SET (ADDITIONAL_DEPENDENCIES
  EnginePluginAssets
)

if (BUILDSYSTEM_PLATFORM_64BIT)

SET (LINK_LIBRARIES
    ${LINK_LIBRARIES}
    debug ${CMAKE_SOURCE_DIR}/Code/ThirdParty/AssImp/lib64/assimp.lib
    optimized ${CMAKE_SOURCE_DIR}/Code/ThirdParty/AssImp/lib64/assimp.lib
)

else()

SET (LINK_LIBRARIES
    ${LINK_LIBRARIES}
    debug ${CMAKE_SOURCE_DIR}/Code/ThirdParty/AssImp/lib32/assimp.lib
    optimized ${CMAKE_SOURCE_DIR}/Code/ThirdParty/AssImp/lib32/assimp.lib
)

endif()

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Plugin.txt)

if (BUILDSYSTEM_PLATFORM_64BIT)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_SOURCE_DIR}/Code/ThirdParty/AssImp/bin64/assimp.dll
          $<TARGET_FILE_DIR:${PROJECT_NAME}>)

else()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_SOURCE_DIR}/Code/ThirdParty/AssImp/bin32/assimp.dll
          $<TARGET_FILE_DIR:${PROJECT_NAME}>)

endif()




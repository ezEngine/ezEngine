#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

if (POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW)
endif()

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
  Utilities
)

SET (ADDITIONAL_DEPENDENCIES
  EnginePluginAssets
)


SET (ASSIMP_DIR "${CMAKE_SOURCE_DIR}/Code/ThirdParty/AssImp")

if (DEFINED BUILDSYSTEM_COMPILER_MSVC_150)
  SET (ASSIMP_DIR "${ASSIMP_DIR}/vc150win")
else()
  SET (ASSIMP_DIR "${ASSIMP_DIR}/vc140win")
endif()

if (${BUILDSYSTEM_PLATFORM_64BIT})
  SET (ASSIMP_DIR "${ASSIMP_DIR}64")
else()
  SET (ASSIMP_DIR "${ASSIMP_DIR}32")
endif()

SET (LINK_LIBRARIES
    ${LINK_LIBRARIES}
    #debug ${ASSIMP_DIR}/assimp-debug.lib
    debug ${ASSIMP_DIR}/assimp.lib
    optimized ${ASSIMP_DIR}/assimp.lib
)

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Plugin.txt)


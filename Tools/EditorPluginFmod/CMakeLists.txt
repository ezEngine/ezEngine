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
BUILD_CONDITIONAL(EZ_BUILD_FMOD)

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Enables the use of Qt in this project and sets up the required modules.
QT_MODULES(Core Gui Widgets)

# Call this in case this project does not function without Qt.
BUILD_REQUIRES_QT()

SET (LINK_LIBRARIES 
  Foundation
  EditorFramework
  ToolsFoundation
  Core
  GameEngine
)

include_directories ("${EZ_FMOD_DIR}/api/studio/inc")
include_directories ("${EZ_FMOD_DIR}/api/fsbank/inc")
include_directories ("${EZ_FMOD_DIR}/api/lowlevel/inc")

link_directories ("${EZ_FMOD_DIR}/api/studio/lib")
link_directories ("${EZ_FMOD_DIR}/api/fsbank/lib")
link_directories ("${EZ_FMOD_DIR}/api/lowlevel/lib")

if (${BUILDSYSTEM_PLATFORM_64BIT})

  SET (LINK_LIBRARIES 
    ${LINK_LIBRARIES}
		fmod64_vc
		fmodstudio64_vc
	)

else()

  SET (LINK_LIBRARIES 
    ${LINK_LIBRARIES}
		fmod_vc
		fmodstudio_vc
	)
endif()

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Plugin.txt)


#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

# Pass a boolean expression into this macro to build this project only if the expression evaluates to true.
BUILD_CONDITIONAL(BUILDSYSTEM_PLATFORM_WINDOWS)
BUILD_CONDITIONAL(EZ_BUILD_FMOD)

set (EZ_FMOD_DIR "" CACHE PATH "Directory of the Fmod SDK")

include_directories ("${EZ_FMOD_DIR}/api/studio/inc")
include_directories ("${EZ_FMOD_DIR}/api/fsbank/inc")
include_directories ("${EZ_FMOD_DIR}/api/lowlevel/inc")

link_directories ("${EZ_FMOD_DIR}/api/studio/lib")
link_directories ("${EZ_FMOD_DIR}/api/fsbank/lib")
link_directories ("${EZ_FMOD_DIR}/api/lowlevel/lib")

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Add all the libraries that this project needs to link against here
# SET (LINK_LIBRARIES
#  Foundation      # Example: Another library build with cmake
#  OpenGL32.lib    # Example: A library that can be found in any lib path, but was not built with cmake
# )

SET (LINK_LIBRARIES
  Foundation
  Core
  GameEngine
)

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

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Library.txt)

add_custom_command(TARGET ${PROJECT_NAME}
  COMMAND ${CMAKE_COMMAND}
	-DMSVC_BUILD_CONFIG_DIR=${CMAKE_CFG_INTDIR}
  -DBUILDSYSTEM_PLATFORM_64BIT=${BUILDSYSTEM_PLATFORM_64BIT}
  -DFMOD_SOURCE=${EZ_FMOD_DIR}
	-DFMOD_TARGET=$<TARGET_FILE_DIR:${PROJECT_NAME}>
	-P "${CMAKE_CURRENT_SOURCE_DIR}/CopyFModDlls.cmake"
  VERBATIM
)

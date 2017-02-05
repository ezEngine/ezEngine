#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

# Pass a boolean expression into this macro to build this project only if the expression evaluates to true.
BUILD_CONDITIONAL(BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
BUILD_CONDITIONAL(EZ_BUILD_PHYSX)

set (EZ_PHYSX_DIR "" CACHE PATH "Directory of PhysX installation")

include_directories ("${EZ_PHYSX_DIR}/Include")

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)


# Add all the libraries that this project needs to link against here
# SET (LINK_LIBRARIES
#  Foundation      # Example: Another library build with cmake
#  OpenGL32.lib    # Example: A library that can be found in any lib path, but was not built with cmake
# )

# this is because PhysX is compiled with that setting
set (COMPILER_SETTINGS_RELEASE ${COMPILER_SETTINGS_RELEASE} "/MT")
set (COMPILER_SETTINGS_DEBUG ${COMPILER_SETTINGS_DEBUG} "/MTd")


SET (LINK_LIBRARIES
  Foundation
  Core
  GameEngine

  debug PhysX3ExtensionsDEBUG
  debug PhysXVisualDebuggerSDKDEBUG
  # debug PxTaskDEBUG
  optimized PhysX3Extensions
  optimized PhysXVisualDebuggerSDK
  # optimized PxTaskCHECKED
)

SET (PHYSX_LIB_DIR "${EZ_PHYSX_DIR}/Lib")
SET (PHYSX_BIN_DIR "${EZ_PHYSX_DIR}/Bin")

if (DEFINED BUILDSYSTEM_COMPILER_MSVC_140)
  SET (PHYSX_LIB_DIR "${PHYSX_LIB_DIR}/vc14")
  SET (PHYSX_BIN_DIR "${PHYSX_BIN_DIR}/vc14")
endif()
if (DEFINED BUILDSYSTEM_COMPILER_MSVC_150)
  SET (PHYSX_LIB_DIR "${PHYSX_LIB_DIR}/vc15")
  SET (PHYSX_BIN_DIR "${PHYSX_BIN_DIR}/vc15")
endif()

if (${BUILDSYSTEM_PLATFORM_64BIT})
  link_directories ("${PHYSX_LIB_DIR}win64")
  
  SET (LINK_LIBRARIES
    ${LINK_LIBRARIES}
    debug PhysX3DEBUG_x64
    debug PhysX3CommonDEBUG_x64
    debug PhysX3CharacterKinematicDEBUG_x64
    optimized PhysX3_x64
    optimized PhysX3Common_x64
    optimized PhysX3CharacterKinematic_x64
  )
  
  SET (PHYSX_BIN_DIR "${PHYSX_BIN_DIR}win64")
else()
  link_directories ("${PHYSX_LIB_DIR}win32")

  SET (LINK_LIBRARIES
    ${LINK_LIBRARIES}
    debug PhysX3DEBUG_x86
    debug PhysX3CommonDEBUG_x86
    debug PhysX3CharacterKinematicDEBUG_x86
    optimized PhysX3_x86
    optimized PhysX3Common_x86
    optimized PhysX3CharacterKinematic_x86
  )
  
  SET (PHYSX_BIN_DIR "${PHYSX_BIN_DIR}win32")
endif()

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Library.txt)


add_custom_command(TARGET ${PROJECT_NAME}
  COMMAND ${CMAKE_COMMAND}
	-DMSVC_BUILD_CONFIG_DIR=${CMAKE_CFG_INTDIR}
  -DBUILDSYSTEM_PLATFORM_64BIT=${BUILDSYSTEM_PLATFORM_64BIT}
  -DPHYSX_SOURCE=${PHYSX_BIN_DIR}
	-DPHYSX_TARGET=$<TARGET_FILE_DIR:${PROJECT_NAME}>
	-P "${CMAKE_CURRENT_SOURCE_DIR}/CopyPhysXDlls.cmake"
  VERBATIM
)

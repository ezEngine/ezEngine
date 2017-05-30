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

include_directories ("${EZ_PHYSX_DIR}/PhysX_3.4/Include")
include_directories ("${EZ_PHYSX_DIR}/PxShared/Include")

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)


# Add all the libraries that this project needs to link against here
# SET (LINK_LIBRARIES
#  Foundation      # Example: Another library build with cmake
#  OpenGL32.lib    # Example: A library that can be found in any lib path, but was not built with cmake
# )


SET (LINK_LIBRARIES
  Foundation
  Core
  GameEngine
  RendererCore

  debug PhysX3ExtensionsDEBUG
  optimized PhysX3Extensions
)

SET (PHYSX_LIB_DIR "${EZ_PHYSX_DIR}/PhysX_3.4/Lib")
SET (PHYSX_BIN_DIR "${EZ_PHYSX_DIR}/PhysX_3.4/Bin")

SET (PXSHARED_LIB_DIR "${EZ_PHYSX_DIR}/PxShared/Lib")
SET (PXSHARED_BIN_DIR "${EZ_PHYSX_DIR}/PxShared/Bin")

if (DEFINED BUILDSYSTEM_COMPILER_MSVC_140)
  SET (PHYSX_LIB_DIR "${PHYSX_LIB_DIR}/vc14")
  SET (PHYSX_BIN_DIR "${PHYSX_BIN_DIR}/vc14")
  
  SET (PXSHARED_LIB_DIR "${PXSHARED_LIB_DIR}/vc14")
  SET (PXSHARED_BIN_DIR "${PXSHARED_BIN_DIR}/vc14")
endif()
if (DEFINED BUILDSYSTEM_COMPILER_MSVC_150)
  SET (PHYSX_LIB_DIR "${PHYSX_LIB_DIR}/vc15")
  SET (PHYSX_BIN_DIR "${PHYSX_BIN_DIR}/vc15")
  
  SET (PXSHARED_LIB_DIR "${PXSHARED_LIB_DIR}/vc15")
  SET (PXSHARED_BIN_DIR "${PXSHARED_BIN_DIR}/vc15")
endif()

if (${BUILDSYSTEM_PLATFORM_64BIT})
  link_directories ("${PHYSX_LIB_DIR}win64")
  link_directories ("${PXSHARED_LIB_DIR}win64")
  
  SET (LINK_LIBRARIES
    ${LINK_LIBRARIES}
    debug PxFoundationDEBUG_x64
    debug PxPvdSDKDEBUG_x64
    debug PhysX3DEBUG_x64
    debug PhysX3CommonDEBUG_x64
    debug PhysX3CharacterKinematicDEBUG_x64
    optimized PxFoundation_x64
    optimized PxPvdSDK_x64
    optimized PhysX3_x64
    optimized PhysX3Common_x64
    optimized PhysX3CharacterKinematic_x64
  )
  
  SET (PHYSX_BIN_DIR "${PHYSX_BIN_DIR}win64")
  SET (PXSHARED_BIN_DIR "${PXSHARED_BIN_DIR}win64")
else()
  link_directories ("${PHYSX_LIB_DIR}win32")
  link_directories ("${PXSHARED_LIB_DIR}win32")

  SET (LINK_LIBRARIES
    ${LINK_LIBRARIES}
    debug PxFoundationDEBUG_x86
    debug PxPvdSDKDEBUG_x86
    debug PhysX3DEBUG_x86
    debug PhysX3CommonDEBUG_x86
    debug PhysX3CharacterKinematicDEBUG_x86
    optimized PxFoundation_x86
    optimized PxPvdSDK_x86
    optimized PhysX3_x86
    optimized PhysX3Common_x86
    optimized PhysX3CharacterKinematic_x86
  )
  
  SET (PHYSX_BIN_DIR "${PHYSX_BIN_DIR}win32")
  SET (PXSHARED_BIN_DIR "${PXSHARED_BIN_DIR}win32")
endif()

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Library.txt)


add_custom_command(TARGET ${PROJECT_NAME}
  COMMAND ${CMAKE_COMMAND}
	-DMSVC_BUILD_CONFIG_DIR=${CMAKE_CFG_INTDIR}
  -DBUILDSYSTEM_PLATFORM_64BIT=${BUILDSYSTEM_PLATFORM_64BIT}
  -DPHYSX_SOURCE=${PHYSX_BIN_DIR}
  -DPXSHARED_SOURCE=${PXSHARED_BIN_DIR}
	-DPHYSX_TARGET=$<TARGET_FILE_DIR:${PROJECT_NAME}>
	-P "${CMAKE_CURRENT_SOURCE_DIR}/CopyPhysXDlls.cmake"
  VERBATIM
)

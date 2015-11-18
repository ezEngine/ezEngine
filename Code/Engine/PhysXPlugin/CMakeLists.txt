#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

# Pass a boolean expression into this macro to build this project only if the expression evaluates to true.
BUILD_CONDITIONAL(BUILDSYSTEM_PLATFORM_WINDOWS)
BUILD_CONDITIONAL(EZ_BUILD_PHYSX)

set (EZ_PHYSX_DIR "" CACHE PATH "Directory of PhysX installation")

include_directories ("${EZ_PHYSX_DIR}/Include")

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Add all the libraries that this project needs to link against here
# SET (LINK_LIBRARIES
#  Foundation      # Example: Another library build with cmake
#  OpenGL32.lib    # Example: A library that can be found in any lib path, but was not built with cmake
# )

# this is because PhysX is compiled with that setting
set (COMPILER_SETTINGS_ALL "${COMPILER_SETTINGS_ALL} /MTd")

SET (LINK_LIBRARIES 
  Foundation
  Core
  CoreUtils
  debug PhysX3ExtensionsDEBUG
  debug PhysXVisualDebuggerSDKDEBUG
  # debug PxTaskDEBUG
  # optimized PhysX3ExtensionsCHECKED
  optimized PhysXVisualDebuggerSDKCHECKED
  # optimized PxTaskCHECKED
)

IF (NOT DEFINED BUILDSYSTEM_COMPILER_MSVC_110)
  message (FATAL_ERROR "Unless you compile PhysX yourself from GitHub sources, PhysX will only compile and link on Visual Studio 2012.")
endif()

if (${BUILDSYSTEM_PLATFORM_64BIT})
  link_directories ("${EZ_PHYSX_DIR}/Lib/vc11win64")

  SET (LINK_LIBRARIES 
    ${LINK_LIBRARIES}
    debug PhysX3DEBUG_x64
    debug PhysX3CommonDEBUG_x64
    debug PhysX3CharacterKinematicDEBUG_x64
    optimized PhysX3CHECKED_x64
    optimized PhysX3CommonCHECKED_x64
    optimized PhysX3CookingCHECKED_x64
    optimized PhysX3CharacterKinematicCHECKED_x64
  )

else()
  link_directories ("${EZ_PHYSX_DIR}/Lib/vc11win32")

  SET (LINK_LIBRARIES 
    ${LINK_LIBRARIES}
    debug PhysX3DEBUG_x86
    debug PhysX3CommonDEBUG_x86
    debug PhysX3CharacterKinematicDEBUG_x86
    optimized PhysX3CHECKED_x86
    optimized PhysX3CommonCHECKED_x86
    optimized PhysX3CookingCHECKED_x86
    optimized PhysX3CharacterKinematicCHECKED_x86
  )
endif()

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Library.txt)
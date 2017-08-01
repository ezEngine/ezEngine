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
)
AddPhysX(FALSE)

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Library.txt)


# Copy dlls for win desktop build
#
# Note that this needs to be done after the library is defined (via CMAKE_Library.txt) which is why this can't be done in CMAKE_PhysX.txt
# (Also, this should happen only in a single one project.)
if (BUILDSYSTEM_PLATFORM_WINDOWS_DESKTOP)
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
 endif()
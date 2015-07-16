#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

BUILD_CONDITIONAL(EZ_BUILD_UNITTESTS)
BUILD_CONDITIONAL(BUILDSYSTEM_PLATFORM_WINDOWS AND BUILDSYSTEM_PLATFORM_64BIT)

# Add all the libraries that this project needs to link against here
SET (LINK_LIBRARIES
  Foundation      # Example: Another library build with cmake
  TestFramework
)

# we want this library to always end up as a static library
set (EZ_COMPILE_ENGINE_AS_DLL 0)

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Library.txt)
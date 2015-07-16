#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

BUILD_CONDITIONAL(EZ_BUILD_UNITTESTS)

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Add all the libraries that this project needs to link against here
SET (LINK_LIBRARIES
  Foundation
  ToolsFoundation
  TestFramework
)

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Application.txt)

# Adds this project to 'Tests.txt', notifying the build server to run this as a test.
# The first parameter sets whether this tests needs direct hardware access to the GPU.
ADD_TEST(OFF)
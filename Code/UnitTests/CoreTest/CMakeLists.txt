#cmake

SET (LINK_LIBRARIES
  ThirdParty
  Foundation
  TestFramework
  Core
  Utilities
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Application.txt")

# Adds this project to 'Tests.txt', notifying the build server to run this as a test.
# The first parameter sets whether this tests needs direct hardware access to the GPU.
ADD_TEST(OFF)

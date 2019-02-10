#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

SET (LINK_LIBRARIES
  Foundation
  Core
  GameEngine
  ThirdParty
  RendererCore
  Utilities
)

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")


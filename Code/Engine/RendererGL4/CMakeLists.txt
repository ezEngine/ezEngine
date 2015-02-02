#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

# Add all the preprocessor definitions that are needed to configure this library
add_definitions (-DEZ_RENDERERGL_GL4)

# Add all the libraries that this project needs to link against here
SET (LINK_LIBRARIES 
  Foundation 
  RendererFoundation
  System
  OpenGL32
)

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Library.txt)
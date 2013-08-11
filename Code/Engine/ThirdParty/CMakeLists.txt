#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# This can be a regular engine library, or an experimental library.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Do not use this file for 'Experimental' libraries ***
#
# *** Modify this file as required ***
#

# Add all the preprocessor definitions that are needed to configure this library
#add_definitions (-DZLIB_INTERNAL)

SET (LINK_LIBRARIES 
  
)

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Library.txt)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CPP)
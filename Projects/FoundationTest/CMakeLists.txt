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
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Add all the libraries that this project needs to link against here
SET (LINK_LIBRARIES
  Foundation      # Example: Another library build with cmake
  TestFramework
)

# Enables the use of Qt in this library.
#SET (EZ_QTPROJECT ON)

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Application.txt)

#if (EZ_ENABLE_QT_SUPPORT)
#  # Define which qt modules are going to be used in this library.
#  qt5_use_modules(${PROJECT_NAME} Widgets)
#endif ()
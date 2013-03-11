#cmake

#
# Copy this file into the folder of an application to add it to the applications solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#


BUILD_APPLICATION_PROJECT()

# Add all the preprocessor definitions that are needed to configure this application
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Add additional include directories, if required
#include_directories ("Stuff/include")

# Add additional library directories, if required
#link_directories ("Stuff/lib")

# add all the library references that the application needs here
set (LINK_LIBRARIES
  Foundation # All projects that are created through cmake can be easily referenced by name
)

# Set this to true, if your application is a real win32 app (not a console app)
#set (IS_WIN32_APPLICATION true)

# execute the "application"-template
include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Application.txt)



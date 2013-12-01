#cmake

#
# Put a copy of this cmake file into the top level folder of an "experimental" project.
# The project will be added to the solution. You automatically get the option to exclude it from the solution via the cmake GUI.
# Also the library-names will be combined with the project name, to make them unique (and thus not collide with any other projects).
#
# *** Do not modify this file. ***
#

# Pass a boolean expression into this macro to build this project only if the expression evaluates to true.
#BUILD_CONDITIONAL(BUILDSYSTEM_PLATFORM_WINDOWS AND NOT BUILDSYSTEM_PLATFORM_64BIT)

# Macro that automatically implement the enabling / disabling of experimental projects.
BUILD_EXPERIMENTAL_PROJECT()

# find all CMakeLists.txt files below this folder
file (GLOB DIRS "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")

# add all those directories to the to-be-processed queue
foreach (VAR ${DIRS})
	get_filename_component (RES ${VAR} PATH)

	# add directory to the queue
	add_subdirectory (${RES})
endforeach ()


#cmake

#
# This is a cmake file that helps to mirror the file structure on disk in Visual Studio.
# This file is identical for all folders in all libraries. By copying it into a folder of a library,
# all files in that folder will automatically be added to the build solution (you might need to re-run cmake though).
#
# *** Do not modify this file. ***
#

# Get the name of this folder
get_filename_component (CUR_FOLDER_NAME ${CMAKE_CURRENT_LIST_DIR} NAME_WE)

# push the folder name
list (APPEND LIST_SOURCE_GROUP_FOLDER ${CUR_FOLDER_NAME})
list (APPEND LIST_CURRENT_FOLDER ${CMAKE_CURRENT_LIST_DIR})

set (TEMP ${EZ_ENABLE_FOLDER_UNITY_FILES})
set (EZ_ENABLE_FOLDER_UNITY_FILES OFF)

	include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_LibraryFolder.txt)

set (EZ_ENABLE_FOLDER_UNITY_FILES ${TEMP})

# pop the folder name
list (REMOVE_AT LIST_SOURCE_GROUP_FOLDER -1)
list (REMOVE_AT LIST_CURRENT_FOLDER -1)
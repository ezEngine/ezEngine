#cmake

# This is a little workaround to import files into the project that are actually not in this directory.

# Get the name of this folder
get_filename_component (CUR_FOLDER_NAME ${CMAKE_CURRENT_LIST_DIR} NAME_WE)

# push the folder name
list (APPEND LIST_CURRENT_FOLDER ${CMAKE_CURRENT_LIST_DIR}/../../RendererGL/)

	include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_LibraryFolder.txt)

# pop the folder name
list (REMOVE_AT LIST_CURRENT_FOLDER -1)
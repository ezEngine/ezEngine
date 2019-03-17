
######################################
### Output directories
######################################

set (EZ_OUTPUT_DIRECTORY_LIB "${CMAKE_SOURCE_DIR}/Output/Lib" CACHE PATH "Where to store the compiled .lib files.")
set (EZ_OUTPUT_DIRECTORY_DLL "${CMAKE_SOURCE_DIR}/Output/Bin" CACHE PATH "Where to store the compiled .dll files.")

mark_as_advanced(FORCE EZ_OUTPUT_DIRECTORY_LIB)
mark_as_advanced(FORCE EZ_OUTPUT_DIRECTORY_DLL)

######################################
### Qt support
######################################

set (EZ_ENABLE_QT_SUPPORT ON CACHE BOOL "Whether to add Qt support.")
set (EZ_QT_DIR $ENV{QTDIR} CACHE PATH "Directory of qt installation")

######################################
### PCH support
######################################

set (EZ_USE_PCH ON CACHE BOOL "Whether to use Precompiled Headers.")

mark_as_advanced(FORCE EZ_USE_PCH)

######################################
### Folder Unity files
######################################

set (EZ_ENABLE_FOLDER_UNITY_FILES ON CACHE BOOL "Whether unity cpp files should be created per folder")

######################################
### PVS Studio support
######################################

set (EZ_ENABLE_PVS_STUDIO_HEADER_IN_UNITY_FILES ON CACHE BOOL "Adds the necessary comment to the generated unity files for PVS checking")

mark_as_advanced(FORCE EZ_ENABLE_PVS_STUDIO_HEADER_IN_UNITY_FILES)

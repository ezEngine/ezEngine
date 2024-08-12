# #####################################
# ## Output directories
# #####################################
set(EZ_OUTPUT_DIRECTORY_LIB "${CMAKE_SOURCE_DIR}/Output/Lib" CACHE PATH "Where to store the compiled .lib files.")
set(EZ_OUTPUT_DIRECTORY_DLL "${CMAKE_SOURCE_DIR}/Output/Bin" CACHE PATH "Where to store the compiled .dll files.")

mark_as_advanced(FORCE EZ_OUTPUT_DIRECTORY_LIB)
mark_as_advanced(FORCE EZ_OUTPUT_DIRECTORY_DLL)

set(EZ_OUTPUT_DIRECTORY_PLATFORM_POSTFIX "" CACHE STRING "Platform postfix for output directory")
mark_as_advanced(FORCE EZ_OUTPUT_DIRECTORY_PLATFORM_POSTFIX)

# #####################################
# ## PCH support
# #####################################
set(EZ_USE_PCH ON CACHE BOOL "Whether to use Precompiled Headers.")

mark_as_advanced(FORCE EZ_USE_PCH)

# #####################################
# ## Folder Unity files
# #####################################
set(EZ_ENABLE_FOLDER_UNITY_FILES ON CACHE BOOL "Whether unity cpp files should be created per folder")

mark_as_advanced(FORCE EZ_ENABLE_FOLDER_UNITY_FILES)

# #####################################
# ## PVS Studio support
# #####################################
set(EZ_ENABLE_PVS_STUDIO_HEADER_IN_UNITY_FILES ON CACHE BOOL "Adds the necessary comment to the generated unity files for PVS checking")

mark_as_advanced(FORCE EZ_ENABLE_PVS_STUDIO_HEADER_IN_UNITY_FILES)

# #####################################
# ## Static analysis support
# #####################################
set(EZ_ENABLE_COMPILER_STATIC_ANALYSIS OFF CACHE BOOL "Enables static analysis in the compiler options")

mark_as_advanced(FORCE EZ_ENABLE_COMPILER_STATIC_ANALYSIS)

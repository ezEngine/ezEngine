
######################################
### Output directories
######################################

set (EZ_OUTPUT_DIRECTORY_LIB "${CMAKE_SOURCE_DIR}/Output/Lib" CACHE PATH "Where to store the compiled .lib files.")
set (EZ_OUTPUT_DIRECTORY_DLL "${CMAKE_SOURCE_DIR}/Output/Bin" CACHE PATH "Where to store the compiled .dll files.")

mark_as_advanced(FORCE EZ_OUTPUT_DIRECTORY_LIB)
mark_as_advanced(FORCE EZ_OUTPUT_DIRECTORY_DLL)

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

######################################
### 3rd Party Code
######################################

### enet
set (EZ_3RDPARTY_ENET_SUPPORT ON CACHE BOOL "Whether to add support for Enet.")
mark_as_advanced(FORCE EZ_3RDPARTY_ENET_SUPPORT)

### Recast
set (EZ_3RDPARTY_RECAST_SUPPORT ON CACHE BOOL "Whether to add support for Recast.")
mark_as_advanced(FORCE EZ_3RDPARTY_RECAST_SUPPORT)

### ImGui
set (EZ_3RDPARTY_IMGUI_SUPPORT ON CACHE BOOL "Whether to add support for ImGui.")
mark_as_advanced(FORCE EZ_3RDPARTY_IMGUI_SUPPORT)

### Lua
set (EZ_3RDPARTY_LUA_SUPPORT ON CACHE BOOL "Whether to add support for Lua.")
mark_as_advanced(FORCE EZ_3RDPARTY_LUA_SUPPORT)

### zstd
set (EZ_3RDPARTY_ZSTD_SUPPORT ON CACHE BOOL "Whether to add support for zstd.")
mark_as_advanced(FORCE EZ_3RDPARTY_ZSTD_SUPPORT)

######################################
### vcpkg
######################################

### Qt
set (EZ_VCPKG_INSTALL_QT OFF CACHE BOOL "Whether to install Qt via vcpkg.")

### PhysX
set (EZ_VCPKG_INSTALL_PHYSX OFF CACHE BOOL "Whether to install PhysX 4 via vcpkg.")

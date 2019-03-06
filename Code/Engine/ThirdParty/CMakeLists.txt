#cmake

if(MSVC)
  if(BUILDSYSTEM_PLATFORM_WINDOWS_UWP)
    SET (LINK_LIBRARIES
      ${LINK_LIBRARIES}
      OneCore.lib
    )
  else()
    SET (LINK_LIBRARIES
      ${LINK_LIBRARIES}
      ws2_32.lib
      Kernel32.lib
      runtimeobject.lib
    )
  endif()
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

if(MSVC)
  target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32)
endif()

#################
## Enet
#################

set (EZ_3RDPARTY_ENET_SUPPORT ON CACHE BOOL "Whether to add support for Enet.")
mark_as_advanced(FORCE EZ_3RDPARTY_ENET_SUPPORT)

if (EZ_3RDPARTY_ENET_SUPPORT)
  target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_ENET_SUPPORT)
endif()

#################
## Lua
#################

set (EZ_3RDPARTY_LUA_SUPPORT ON CACHE BOOL "Whether to add support for Lua.")
mark_as_advanced(FORCE EZ_3RDPARTY_LUA_SUPPORT)

if (EZ_3RDPARTY_LUA_SUPPORT)
  target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_LUA_SUPPORT)
endif()

#################
## Zlib
#################

set (EZ_3RDPARTY_ZLIB_SUPPORT OFF CACHE BOOL "Whether to add support for zlib.")
mark_as_advanced(FORCE EZ_3RDPARTY_ZLIB_SUPPORT)

if (EZ_3RDPARTY_ZLIB_SUPPORT)
  target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_ZLIB_SUPPORT)
endif()

#################
## Zstandard
#################

set (EZ_3RDPARTY_ZSTD_SUPPORT ON CACHE BOOL "Whether to add support for zstd.")
mark_as_advanced(FORCE EZ_3RDPARTY_ZSTD_SUPPORT)

if (EZ_3RDPARTY_ZSTD_SUPPORT)
  target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_ZSTD_SUPPORT)
endif()

#################
#################


# This will break clang makefile builds:
if(MSVC)
  set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CPP)
endif()

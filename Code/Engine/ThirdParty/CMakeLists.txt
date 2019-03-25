ez_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME} EXCLUDE_FOLDER_FOR_UNITY Recast)

if(MSVC)
  target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32)

  if(EZ_CMAKE_PLATFORM_WINDOWS_UWP)

    target_link_libraries(${PROJECT_NAME}
      PRIVATE
      OneCore.lib
    )

  else()

    target_link_libraries(${PROJECT_NAME}
      PRIVATE
      ws2_32.lib
      Kernel32.lib
      runtimeobject.lib
    )

  endif()
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
#################


# This will break clang makefile builds:
if(MSVC)
  set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CPP)
endif()

ez_cmake_init()

ez_requires(EZ_3RDPARTY_ENET_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME} NO_EZ_PREFIX)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_ENET_SUPPORT)

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

ez_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

if(EZ_CMAKE_PLATFORM_WINDOWS)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE

    Rpcrt4.lib
  )
endif()

ez_enable_strict_warnings(${PROJECT_NAME})

if(EZ_CMAKE_COMPILER_CLANG)
  set_source_files_properties(ThirdParty/xxHash/xxHash.c PROPERTIES COMPILE_FLAGS "-Wno-unused-function")
endif()

ez_target_link_livepp(${PROJECT_NAME})

if (EZ_CMAKE_PLATFORM_LINUX)
  CHECK_INCLUDE_FILE_CXX("uuid/uuid.h" UUID_HEADER)

  if(NOT UUID_HEADER)
    message(FATAL_ERROR "Missing uuid/uuid.h. Have you installed the uuid-dev package?")
  endif()

  target_link_libraries(${PROJECT_NAME}
    PRIVATE

    uuid
    dl # dlopen, dlclose, etc
  )
endif()

if(EZ_CMAKE_PLATFORM_OSX)
  find_library(CORESERVICES_LIBRARY CoreServices)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)

  mark_as_advanced(FORCE CORESERVICES_LIBRARY COREFOUNDATION_LIBRARY)

  target_link_libraries(${PROJECT_NAME}
    PRIVATE

    ${CORESERVICES_LIBRARY}
    ${COREFOUNDATION_LIBRARY}
  )
endif()

if(EZ_3RDPARTY_ENET_SUPPORT)
  target_link_libraries(${PROJECT_NAME} PUBLIC enet)
endif()

if(EZ_3RDPARTY_ZSTD_SUPPORT)
  target_link_libraries(${PROJECT_NAME} PUBLIC zstd)
endif()

if(EZ_3RDPARTY_ZLIB_SUPPORT)
  # ZLIB is required to read the apk on Android.
  target_link_libraries(${PROJECT_NAME} PUBLIC zlib)
endif()

if(EZ_3RDPARTY_GLFW_SUPPORT)
  target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_GLFW_SUPPORT)
  target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
endif()

if(EZ_3RDPARTY_TRACELOGGING_LTTNG_SUPPORT)
  target_link_libraries(${PROJECT_NAME} PRIVATE tracelogging)
endif()

if(EZ_3RDPARTY_TRACY_SUPPORT)
  # Tracy has to be linked to all possible targets since it is a multi dll project.
  target_link_libraries(${PROJECT_NAME} PUBLIC TracyClient)
endif()
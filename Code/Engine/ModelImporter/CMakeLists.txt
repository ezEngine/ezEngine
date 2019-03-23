ez_cmake_init()

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  RendererCore
)

# deactivated for now, would need to be updated to modern CMake
#set (EZ_BUILD_OFFICIAL_FBX_SDK OFF CACHE BOOL "Whether to use the official Autodesk FBX SDK or not")
#mark_as_advanced(FORCE EZ_BUILD_OFFICIAL_FBX_SDK)
#
#if (EZ_BUILD_OFFICIAL_FBX_SDK)
#
#    set (EZ_FBXSDK_DIR "" CACHE PATH "Directory of Autodesk FBX SDK installation")
#    target_include_directories (${PROJECT_NAME} PRIVATE "${EZ_FBXSDK_DIR}/include")
#
#    set (FBXSDK_LIB_DIR "${EZ_FBXSDK_DIR}/lib")
#
#    if (DEFINED BUILDSYSTEM_COMPILER_MSVC_140 OR DEFINED BUILDSYSTEM_COMPILER_MSVC_141)
#      if (${BUILDSYSTEM_PLATFORM_64BIT})
#        set (FBXSDK_LIB_DIR "${FBXSDK_LIB_DIR}/vs2015/x64")
#      else()
#        set (FBXSDK_LIB_DIR "${FBXSDK_LIB_DIR}/vs2015/x86")
#      endif()
#    endif()
#
#    set (LINK_LIBRARIES
#    ${LINK_LIBRARIES}
#    debug ${FBXSDK_LIB_DIR}/debug/libfbxsdk-md.lib
#    optimized ${FBXSDK_LIB_DIR}/release/libfbxsdk-md.lib
#    )
#
#    add_definitions(-DBUILDSYSTEM_BUILD_WITH_OFFICIAL_FBX_SDK)
#
#endif()

find_package(EzAssImp REQUIRED)

if (EZASSIMP_FOUND)

  target_link_libraries(${PROJECT_NAME} PRIVATE EzAssImp::EzAssImp)

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:EzAssImp::EzAssImp> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )

endif()

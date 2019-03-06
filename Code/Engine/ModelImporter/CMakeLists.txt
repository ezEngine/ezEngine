#cmake

SET (LINK_LIBRARIES
  Foundation
  RendererCore
  ThirdParty
)

set (EZ_BUILD_OFFICIAL_FBX_SDK OFF CACHE BOOL "Whether to use the official Autodesk FBX SDK or not")
mark_as_advanced(FORCE EZ_BUILD_OFFICIAL_FBX_SDK)

if (EZ_BUILD_OFFICIAL_FBX_SDK)

    set (EZ_FBXSDK_DIR "" CACHE PATH "Directory of Autodesk FBX SDK installation")
    include_directories ("${EZ_FBXSDK_DIR}/include")

    SET (FBXSDK_LIB_DIR "${EZ_FBXSDK_DIR}/lib")

    if (DEFINED BUILDSYSTEM_COMPILER_MSVC_140 OR DEFINED BUILDSYSTEM_COMPILER_MSVC_141)
      if (${BUILDSYSTEM_PLATFORM_64BIT})
        SET (FBXSDK_LIB_DIR "${FBXSDK_LIB_DIR}/vs2015/x64")
      else()
        SET (FBXSDK_LIB_DIR "${FBXSDK_LIB_DIR}/vs2015/x86")
      endif()
    endif()

    SET (LINK_LIBRARIES
    ${LINK_LIBRARIES}
    debug ${FBXSDK_LIB_DIR}/debug/libfbxsdk-md.lib
    optimized ${FBXSDK_LIB_DIR}/release/libfbxsdk-md.lib
    )

    add_definitions(-DBUILDSYSTEM_BUILD_WITH_OFFICIAL_FBX_SDK)

endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

find_package(EzAssImp REQUIRED)

if (EZASSIMP_FOUND)

  target_link_libraries(${PROJECT_NAME} EzAssImp::EzAssImp)

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:EzAssImp::EzAssImp> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )

endif()

#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Add all the libraries that this project needs to link against here
# SET (LINK_LIBRARIES
#  Foundation      # Example: Another library build with cmake
#  OpenGL32.lib    # Example: A library that can be found in any lib path, but was not built with cmake
# )

SET (LINK_LIBRARIES
  Foundation
  RendererCore
  ThirdParty
)

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

if (BUILDSYSTEM_PLATFORM_64BIT)

SET (LINK_LIBRARIES
    ${LINK_LIBRARIES}
    debug ${CMAKE_SOURCE_DIR}/Code/ThirdParty/AssImp/lib64/assimp.lib
    optimized ${CMAKE_SOURCE_DIR}/Code/ThirdParty/AssImp/lib64/assimp.lib
)

else()

SET (LINK_LIBRARIES
    ${LINK_LIBRARIES}
    debug ${CMAKE_SOURCE_DIR}/Code/ThirdParty/AssImp/lib32/assimp.lib
    optimized ${CMAKE_SOURCE_DIR}/Code/ThirdParty/AssImp/lib32/assimp.lib
)

endif()

include (${CMAKE_SOURCE_DIR}/Code/BuildSystem/CMake/CMAKE_Library.txt)

if (BUILDSYSTEM_PLATFORM_64BIT)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_SOURCE_DIR}/Code/ThirdParty/AssImp/bin64/assimp.dll
          $<TARGET_FILE_DIR:${PROJECT_NAME}>)

else()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_SOURCE_DIR}/Code/ThirdParty/AssImp/bin32/assimp.dll
          $<TARGET_FILE_DIR:${PROJECT_NAME}>)

endif()



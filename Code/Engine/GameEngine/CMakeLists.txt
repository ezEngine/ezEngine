#cmake


include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  RendererDX11
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  System
  RendererCore
)

if (BUILDSYSTEM_PLATFORM_WINDOWS_UWP)

  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    FileservePlugin
  )

endif()

if (BUILDSYSTEM_PLATFORM_WINDOWS_UWP AND EZ_ENABLE_MIXEDREALITY_SUPPORT)

  target_link_libraries(${PROJECT_NAME}
    PRIVATE
    WindowsMixedReality
  )

endif()

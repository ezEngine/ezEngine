ez_pull_all_vars()

if (EZ_CMAKE_PLATFORM_WINDOWS_UWP AND TARGET TestFramework AND TARGET FileservePlugin)
    target_link_libraries(TestFramework PRIVATE FileservePlugin)
    target_compile_definitions (TestFramework PRIVATE EZ_TESTFRAMEWORK_USE_FILESERVE)
endif()

if(EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)
    set (EZ_3RDPARTY_TRACY_SUPPORT ON CACHE BOOL "Whether to add support for profiling the engine with Tracy.")
else()
    # Tracy currently doesn't compile on Linux
    set (EZ_3RDPARTY_TRACY_SUPPORT OFF CACHE BOOL "Whether to add support for profiling the engine with Tracy.")
endif()

mark_as_advanced(FORCE EZ_3RDPARTY_TRACY_SUPPORT)
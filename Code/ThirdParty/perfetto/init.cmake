ez_requires(EZ_CMAKE_PLATFORM_ANDROID)

set (EZ_3RDPARTY_PERFETTO_SUPPORT ON CACHE BOOL "Whether to add support for Perfetto tracing on Android.")
mark_as_advanced(FORCE EZ_3RDPARTY_PERFETTO_SUPPORT)

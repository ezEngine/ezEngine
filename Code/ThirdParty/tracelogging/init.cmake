ez_requires(EZ_CMAKE_PLATFORM_LINUX)

set (EZ_3RDPARTY_TRACELOGGING_LTTNG_SUPPORT OFF CACHE BOOL "Whether to add support for tracelogging via lttng.")
mark_as_advanced(FORCE EZ_3RDPARTY_TRACELOGGING_LTTNG_SUPPORT)
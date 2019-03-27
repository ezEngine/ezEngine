set (EZ_BUILD_SAMPLES ON CACHE BOOL "Whether sample projects should be added to the solution")

ez_requires(EZ_BUILD_SAMPLES)

ez_add_all_subdirs()
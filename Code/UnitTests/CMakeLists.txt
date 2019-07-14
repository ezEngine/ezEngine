set (EZ_BUILD_UNITTESTS ON CACHE BOOL "Whether unit-test projects should be added to the solution")

ez_requires(EZ_BUILD_UNITTESTS)

ez_add_all_subdirs()
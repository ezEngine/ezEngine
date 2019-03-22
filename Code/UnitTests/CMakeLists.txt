set (EZ_BUILD_UNITTESTS ON CACHE BOOL "Whether unit-test projects should be added to the solution")

if (NOT EZ_BUILD_UNITTESTS)
	return()
endif()

ez_add_all_subdirs()
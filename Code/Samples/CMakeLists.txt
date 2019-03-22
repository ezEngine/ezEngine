set (EZ_BUILD_SAMPLES ON CACHE BOOL "Whether sample projects should be added to the solution")

if (NOT EZ_BUILD_SAMPLES)
	return()
endif()

ez_add_all_subdirs()
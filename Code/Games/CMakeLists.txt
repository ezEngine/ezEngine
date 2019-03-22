set (EZ_BUILD_GAMES OFF CACHE BOOL "Whether game projects should be added to the solution")

if (NOT EZ_BUILD_GAMES)
	return()
endif()

ez_add_all_subdirs()
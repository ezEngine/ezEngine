set (EZ_BUILD_GAMES OFF CACHE BOOL "Whether game projects should be added to the solution")

ez_requires(${EZ_BUILD_GAMES})

ez_add_all_subdirs()
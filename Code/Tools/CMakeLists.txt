

set (EZ_BUILD_TOOLS ON CACHE BOOL "Whether tool projects should be added to the solution")

ez_requires(EZ_BUILD_TOOLS)

ez_add_all_subdirs()
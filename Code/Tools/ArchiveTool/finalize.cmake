ez_pull_all_vars()

if (TARGET FoundationTest AND TARGET ArchiveTool)
  if (EZ_CMAKE_PLATFORM_WINDOWS_DESKTOP)
    add_dependencies(FoundationTest ArchiveTool)
  endif()
endif()
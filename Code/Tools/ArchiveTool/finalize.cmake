ez_pull_all_vars()

if (TARGET FoundationTest AND TARGET ArchiveTool)
  add_dependencies(FoundationTest ArchiveTool)
endif()
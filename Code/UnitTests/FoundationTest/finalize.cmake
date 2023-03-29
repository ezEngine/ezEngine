if (TARGET FoundationTest AND TARGET ArchiveTool)

	target_compile_definitions(FoundationTest PRIVATE BUILDSYSTEM_HAS_ARCHIVE_TOOL)

    # Make sure this project is built when the test is built
    add_dependencies(FoundationTest ArchiveTool)

endif()

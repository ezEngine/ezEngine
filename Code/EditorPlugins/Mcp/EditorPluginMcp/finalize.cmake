if (TARGET Editor AND TARGET EditorPluginMcp)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginMcp)

endif()

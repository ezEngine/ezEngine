if (TARGET Editor AND TARGET EditorPluginTypeScript)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginTypeScript)

endif()

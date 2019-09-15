if (TARGET Editor AND TARGET EditorPluginScene)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginScene)

endif()

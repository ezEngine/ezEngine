if (TARGET Editor AND TARGET EditorPluginVisualScript)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginVisualScript)

endif()

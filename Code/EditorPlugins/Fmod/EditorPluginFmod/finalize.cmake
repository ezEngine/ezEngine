if (TARGET Editor AND TARGET EditorPluginFmod)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginFmod)

endif()

if (TARGET Editor AND TARGET EditorPluginRecast)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginRecast)

endif()

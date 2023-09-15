if (TARGET Editor AND TARGET EditorPluginAi)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginAi)

endif()

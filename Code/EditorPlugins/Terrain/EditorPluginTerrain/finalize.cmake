if (TARGET Editor AND TARGET EditorPluginTerrain)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginTerrain)

endif()

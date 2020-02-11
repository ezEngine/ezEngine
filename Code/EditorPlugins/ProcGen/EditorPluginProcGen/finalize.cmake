if (TARGET Editor AND TARGET EditorPluginProcGen)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginProcGen)

endif()

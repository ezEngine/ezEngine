if (TARGET Editor AND TARGET EditorPluginFileserve)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginFileserve)

endif()

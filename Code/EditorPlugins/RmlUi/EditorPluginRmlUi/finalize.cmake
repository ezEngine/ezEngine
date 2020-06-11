if (TARGET Editor AND TARGET EditorPluginRmlUi)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginRmlUi)

endif()

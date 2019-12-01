if (TARGET Editor AND TARGET EditorPluginUltralight)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginUltralight)

endif()

if (TARGET Editor AND TARGET EditorPluginAssets)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginAssets)

endif()

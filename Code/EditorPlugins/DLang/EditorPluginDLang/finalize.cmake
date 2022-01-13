if (TARGET Editor AND TARGET EditorPluginDLang)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginDLang)

endif()

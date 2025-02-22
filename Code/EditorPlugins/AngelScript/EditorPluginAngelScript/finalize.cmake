if (TARGET Editor AND TARGET EditorPluginAngelScript)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginAngelScript)

endif()

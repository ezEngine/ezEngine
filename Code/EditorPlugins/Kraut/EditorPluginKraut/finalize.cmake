if (TARGET Editor AND TARGET EditorPluginKraut)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginKraut)

endif()

if (TARGET Editor AND TARGET EditorPluginJolt)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginJolt)

endif()

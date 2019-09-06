if (TARGET Editor AND TARGET EditorPluginPhysX)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginPhysX)

endif()

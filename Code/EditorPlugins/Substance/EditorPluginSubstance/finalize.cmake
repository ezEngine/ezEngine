if (TARGET Editor AND TARGET EditorPluginSubstance)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginSubstance)

endif()

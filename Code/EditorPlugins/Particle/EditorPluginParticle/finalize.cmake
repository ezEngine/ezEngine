if (TARGET Editor AND TARGET EditorPluginParticle)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginParticle)

endif()

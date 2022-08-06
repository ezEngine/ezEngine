if (TARGET Editor AND TARGET EditorPluginAudioSystem)
    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginAudioSystem)
endif()

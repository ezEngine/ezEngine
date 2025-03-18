if (TARGET Editor AND TARGET EditorPluginMiniAudio)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginMiniAudio)

endif()

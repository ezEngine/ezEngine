if (TARGET Editor AND TARGET SampleGamePlugin)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor SampleGamePlugin)

endif()

if (TARGET Editor AND TARGET RtsGamePlugin)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor RtsGamePlugin)

endif()

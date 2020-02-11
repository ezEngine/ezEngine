if (TARGET Editor AND TARGET RenderDocPlugin)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor RenderDocPlugin)

endif()

if (TARGET Player AND TARGET RmlUiPlugin)

    # Make sure this project is built when the Editor is built
    add_dependencies(Player RmlUiPlugin)

endif()

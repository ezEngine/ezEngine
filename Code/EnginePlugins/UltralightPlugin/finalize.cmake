if (TARGET Editor AND TARGET UltralightPlugin)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor UltralightPlugin)

endif()

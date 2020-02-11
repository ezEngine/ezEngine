if (TARGET Editor AND TARGET TypeScriptPlugin)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor TypeScriptPlugin)

endif()

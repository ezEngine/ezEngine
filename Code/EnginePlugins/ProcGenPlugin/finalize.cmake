if (TARGET Player AND TARGET ProcGenPlugin)

    # Make sure this project is built when the Editor is built
    add_dependencies(Player ProcGenPlugin)

endif()

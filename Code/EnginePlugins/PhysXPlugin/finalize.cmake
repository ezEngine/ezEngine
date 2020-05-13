if (TARGET Player AND TARGET PhysXPlugin)

    # Make sure this project is built when the Editor is built
    add_dependencies(Player PhysXPlugin)

endif()

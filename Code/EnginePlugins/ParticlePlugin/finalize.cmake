if (TARGET Player AND TARGET ParticlePlugin)

    # Make sure this project is built when the Editor is built
    add_dependencies(Player ParticlePlugin)

endif()

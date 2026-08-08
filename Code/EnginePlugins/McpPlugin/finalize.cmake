if (TARGET Player AND TARGET McpPlugin)

    # The plugin is loaded at runtime through the plugin bundle, so nothing links against it and the
    # build system would otherwise be free to skip it. ezPlayer without it has no MCP server at all.
    add_dependencies(Player McpPlugin)

endif()

if (TARGET Editor AND TARGET McpPlugin)

    # Same reason, for the engine process that the editor starts.
    add_dependencies(Editor McpPlugin)

endif()

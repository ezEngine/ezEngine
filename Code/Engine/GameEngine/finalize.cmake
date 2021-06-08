if (TARGET RendererDX11)
  add_dependencies(GameEngine RendererDX11)
endif()

if (TARGET RendererVulkan)
  add_dependencies(GameEngine RendererVulkan)
endif()

if (TARGET RendererDX11)
  add_dependencies(TextureSample RendererDX11)
endif()

if (TARGET RendererVulkan)
  add_dependencies(TextureSample RendererVulkan)
endif()

#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// Forward render pass that renders skybox and sky objects.
///
/// Renders the skybox and other sky-related objects at infinite distance.
/// Typically rendered after opaque objects but before transparent objects.
class EZ_RENDERERCORE_DLL ezSkyRenderPass : public ezForwardRenderPass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkyRenderPass, ezForwardRenderPass);

public:
  ezSkyRenderPass(const char* szName = "SkyRenderPass");
  ~ezSkyRenderPass();

protected:
  virtual void DeclareRenderObjectDependencies(ezRenderGraph& ref_graph, ezRenderGraphPassBuilder& ref_pass) override;
  virtual void RenderObjects(const ezRenderViewContext& renderViewContext) override;
};

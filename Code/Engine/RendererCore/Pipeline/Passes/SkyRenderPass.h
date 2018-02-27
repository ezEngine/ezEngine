#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// \brief A forward render pass that renders all sky objects into the color target.
class EZ_RENDERERCORE_DLL ezSkyRenderPass : public ezForwardRenderPass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkyRenderPass, ezForwardRenderPass);

public:
  ezSkyRenderPass(const char* szName = "SkyRenderPass");
  ~ezSkyRenderPass();

protected:

  virtual void RenderObjects(const ezRenderViewContext& renderViewContext) override;
};

#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// Forward render pass that renders secondary lens flares.
class EZ_RENDERERCORE_DLL ezLensEffectsPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLensEffectsPass, ezRenderPipelinePass);

public:
  ezLensEffectsPass(const char* szName = "LensEffectsPass");
  ~ezLensEffectsPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

protected:
  ezRenderPipelineNodePassThroughPin m_PinColor;
  ezRenderPipelineNodeInputPin m_PinResolvedDepth;
};

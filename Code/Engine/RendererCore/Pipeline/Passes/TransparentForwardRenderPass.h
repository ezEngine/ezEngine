#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// \brief A forward render pass that renders all transparent objects into the color target.
class EZ_RENDERERCORE_DLL ezTransparentForwardRenderPass : public ezForwardRenderPass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTransparentForwardRenderPass, ezForwardRenderPass);

public:
  ezTransparentForwardRenderPass(const char* szName = "TransparentForwardRenderPass");
  ~ezTransparentForwardRenderPass();

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

protected:
  virtual void SetupResources(ezGALPass* pGALPass, const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual void RenderObjects(const ezRenderViewContext& renderViewContext) override;

  void UpdateSceneColorTexture(const ezRenderViewContext& renderViewContext, ezGALTextureHandle hSceneColorTexture, ezGALTextureHandle hCurrentColorTexture);
  void CreateSamplerState();

  ezRenderPipelineNodeInputPin m_PinResolvedDepth;

  ezGALSamplerStateHandle m_hSceneColorSamplerState;
};

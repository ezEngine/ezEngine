#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// Forward render pass that renders transparent objects with proper blending.
///
/// Provides access to the scene color for refraction and distortion effects.
class EZ_RENDERERCORE_DLL ezTransparentForwardRenderPass : public ezForwardRenderPass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTransparentForwardRenderPass, ezForwardRenderPass);

public:
  ezTransparentForwardRenderPass(const char* szName = "TransparentForwardRenderPass");
  ~ezTransparentForwardRenderPass();

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

protected:
  virtual void SetupResources(ezGALCommandEncoder* pCommandEncoder, const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual void RenderObjects(const ezRenderViewContext& renderViewContext) override;

  /// Copies the current scene color for use in transparent shaders (refraction, etc.).
  void UpdateSceneColorTexture(const ezRenderViewContext& renderViewContext, ezGALTextureHandle hSceneColorTexture, ezGALTextureHandle hCurrentColorTexture);

  void CreateSamplerState();

  ezRenderPipelineNodeInputPin m_PinResolvedDepth;   ///< Optional resolved depth for soft particles.

  ezGALSamplerStateHandle m_hSceneColorSamplerState; ///< Sampler for scene color texture.
};

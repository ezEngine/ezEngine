#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// Forward render pass that renders opaque objects.
///
/// Renders all opaque geometry with full lighting and shading.
/// Optionally accepts an ambient occlusion input for enhanced shading.
class EZ_RENDERERCORE_DLL ezOpaqueForwardRenderPass : public ezForwardRenderPass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezOpaqueForwardRenderPass, ezForwardRenderPass);

public:
  ezOpaqueForwardRenderPass(const char* szName = "OpaqueForwardRenderPass");
  ~ezOpaqueForwardRenderPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

protected:
  virtual void SetupResources(ezGALCommandEncoder* pCommandEncoder, const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual void SetupPermutationVars(const ezRenderViewContext& renderViewContext) override;

  virtual void RenderObjects(const ezRenderViewContext& renderViewContext) override;

  ezRenderPipelineNodeInputPin m_PinSSAO;    ///< Optional SSAO input for ambient occlusion.

  bool m_bWriteDepth = true;                 ///< Whether to write to the depth buffer.

  ezTexture2DResourceHandle m_hWhiteTexture; ///< Fallback white texture for unbound inputs.
};

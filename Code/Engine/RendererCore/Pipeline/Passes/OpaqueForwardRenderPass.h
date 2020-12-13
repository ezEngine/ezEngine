#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// \brief A forward render pass that renders all opaque objects into the color target.
class EZ_RENDERERCORE_DLL ezOpaqueForwardRenderPass : public ezForwardRenderPass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezOpaqueForwardRenderPass, ezForwardRenderPass);

public:
  ezOpaqueForwardRenderPass(const char* szName = "OpaqueForwardRenderPass");
  ~ezOpaqueForwardRenderPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

protected:
  virtual void SetupResources(ezGALPass* pGALPass, const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual void SetupPermutationVars(const ezRenderViewContext& renderViewContext) override;

  virtual void RenderObjects(const ezRenderViewContext& renderViewContext) override;

  ezRenderPipelineNodeInputPin m_PinSSAO;
  // ezRenderPipelineNodeOutputPin m_PinNormal;
  // ezRenderPipelineNodeOutputPin m_PinSpecularColorRoughness;

  bool m_bWriteDepth;

  ezTexture2DResourceHandle m_hWhiteTexture;
};

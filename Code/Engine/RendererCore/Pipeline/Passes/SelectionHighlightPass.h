#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

class EZ_RENDERERCORE_DLL ezSelectionHighlightPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSelectionHighlightPass, ezRenderPipelinePass);

public:
  ezSelectionHighlightPass(const char* szName = "SelectionHighlightPass");
  ~ezSelectionHighlightPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

protected:
  ezPassThroughNodePin m_PinColor;
  ezInputNodePin m_PinDepthStencil;

  ezShaderResourceHandle m_hShader;
  ezConstantBufferStorageHandle m_hConstantBuffer;

  ezColor m_HighlightColor;
  float m_fOverlayOpacity;
};


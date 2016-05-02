#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

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
  ezPassThroughNodePin m_PinDepthStencil;

  ezGALSamplerStateHandle m_hSamplerState;
  ezShaderResourceHandle m_hShader;
};

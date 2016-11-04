#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

class EZ_RENDERERCORE_DLL ezMsaaUpscalePass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMsaaUpscalePass, ezRenderPipelinePass);

public:
  ezMsaaUpscalePass();
  ~ezMsaaUpscalePass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

protected:
  ezInputNodePin m_PinInput;
  ezOutputNodePin m_PinOutput;

  ezGALMSAASampleCount::Enum m_MsaaMode;
  ezShaderResourceHandle m_hShader;
};

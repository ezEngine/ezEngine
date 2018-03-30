#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

class EZ_RENDERERCORE_DLL ezMsaaResolvePass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMsaaResolvePass, ezRenderPipelinePass);

public:
  ezMsaaResolvePass();
  ~ezMsaaResolvePass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

protected:
  ezInputNodePin m_PinInput;
  ezOutputNodePin m_PinOutput;

  bool m_bIsDepth;
  ezGALMSAASampleCount::Enum m_MsaaSampleCount;
  ezShaderResourceHandle m_hDepthResolveShader;
};


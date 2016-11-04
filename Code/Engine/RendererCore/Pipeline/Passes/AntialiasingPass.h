#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

class EZ_RENDERERCORE_DLL ezAntialiasingPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAntialiasingPass, ezRenderPipelinePass);

public:
  ezAntialiasingPass();
  ~ezAntialiasingPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

protected:
  ezInputNodePin m_PinInput;
  ezOutputNodePin m_PinOutput;

  ezHashedString m_sMsaaSampleCount;
  ezShaderResourceHandle m_hShader;
};

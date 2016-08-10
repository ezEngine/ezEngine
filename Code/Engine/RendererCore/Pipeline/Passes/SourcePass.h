#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

class EZ_RENDERERCORE_DLL ezSourcePass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSourcePass, ezRenderPipelinePass);

public:
  ezSourcePass(const char* szName = "SourcePass");
  ~ezSourcePass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

protected:
  ezOutputNodePin m_PinOutput;

  ezGALResourceFormat::Enum m_Format;
  ezGALMSAASampleCount::Enum m_MsaaMode;
  ezColor m_ClearColor;
  bool m_bClear;
};

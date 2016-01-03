#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

class EZ_RENDERERCORE_DLL ezTargetPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTargetPass, ezRenderPipelinePass);

public:
  ezTargetPass(const char* szName = "TargetPass");
  ~ezTargetPass();

  ezGALTextureHandle GetTextureHandle(const ezNodePin* pPin);

  virtual bool GetRenderTargetDescriptions(const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void SetRenderTargets(const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext) override;

private:
  bool VerifyInput(const ezArrayPtr<ezGALTextureCreationDescription*const> inputs, const char* szPinName);

protected:
  ezInputNodePin m_PinColor0;
  ezInputNodePin m_PinColor1;
  ezInputNodePin m_PinColor2;
  ezInputNodePin m_PinColor3;
  ezInputNodePin m_PinColor4;
  ezInputNodePin m_PinColor5;
  ezInputNodePin m_PinColor6;
  ezInputNodePin m_PinColor7;
  ezInputNodePin m_PinDepthStencil;
};

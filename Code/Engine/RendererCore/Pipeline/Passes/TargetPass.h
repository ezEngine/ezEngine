#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

struct ezGALRenderTargets;

class EZ_RENDERERCORE_DLL ezTargetPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTargetPass, ezRenderPipelinePass);

public:
  ezTargetPass(const char* szName = "TargetPass");
  ~ezTargetPass();

  const ezGALTextureHandle* GetTextureHandle(const ezGALRenderTargets& renderTargets, const ezRenderPipelineNodePin* pPin);

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

private:
  bool VerifyInput(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, const char* szPinName);

protected:
  ezRenderPipelineNodeInputPin m_PinColor0;
  ezRenderPipelineNodeInputPin m_PinColor1;
  ezRenderPipelineNodeInputPin m_PinColor2;
  ezRenderPipelineNodeInputPin m_PinColor3;
  ezRenderPipelineNodeInputPin m_PinColor4;
  ezRenderPipelineNodeInputPin m_PinColor5;
  ezRenderPipelineNodeInputPin m_PinColor6;
  ezRenderPipelineNodeInputPin m_PinColor7;
  ezRenderPipelineNodeInputPin m_PinDepthStencil;
};

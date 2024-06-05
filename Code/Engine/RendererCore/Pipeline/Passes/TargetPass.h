#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

struct ezGALRenderTargets;

class EZ_RENDERERCORE_DLL ezTargetPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTargetPass, ezRenderPipelinePass);

public:
  ezTargetPass(const char* szName = "TargetPass");
  ~ezTargetPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual ezGALTextureHandle QueryTextureProvider(const ezRenderPipelineNodePin* pPin, const ezGALTextureCreationDescription& desc) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

private:
  bool VerifyInput(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, const char* szPinName);

protected:
  ezRenderPipelineNodeInputProviderPin m_PinColor0;
  ezRenderPipelineNodeInputProviderPin m_PinColor1;
  ezRenderPipelineNodeInputProviderPin m_PinColor2;
  ezRenderPipelineNodeInputProviderPin m_PinColor3;
  ezRenderPipelineNodeInputProviderPin m_PinColor4;
  ezRenderPipelineNodeInputProviderPin m_PinColor5;
  ezRenderPipelineNodeInputProviderPin m_PinColor6;
  ezRenderPipelineNodeInputProviderPin m_PinColor7;
  ezRenderPipelineNodeInputProviderPin m_PinDepthStencil;

  ezGALRenderTargets m_renderTargets;
  ezGALSwapChainHandle m_hSwapChain;
};

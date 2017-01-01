#pragma once

#include <StochasticRendering/Basics.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;

class EZ_STOCHASTICRENDERING_DLL ezStochasticResolvePass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStochasticResolvePass, ezRenderPipelinePass);

public:

  ezStochasticResolvePass(const char* name = "StoachsticResolvePass");
  ~ezStochasticResolvePass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription * const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection * const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection * const> outputs) override;

private:
  ezPassThroughNodePin m_PinColor;
  ezInputNodePin m_PinStochasticColor;
  ezInputNodePin m_PinSampleCount;

  ezShaderResourceHandle m_hResolveShader;
};

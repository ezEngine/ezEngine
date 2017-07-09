#pragma once

#include <StochasticRendering/Basics.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Math/Random.h>

class EZ_STOCHASTICRENDERING_DLL ezStochasticPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStochasticPass, ezRenderPipelinePass);

public:

  ezStochasticPass(const char* name = "StoachsticPass");
  ~ezStochasticPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription * const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection * const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection * const> outputs) override;

  virtual void InitRenderPipelinePass(const ezArrayPtr<ezRenderPipelinePassConnection * const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection * const> outputs) override;

private:
  ezPassThroughNodePin m_PinColor;
  ezPassThroughNodePin m_PinSampleCount;
  ezPassThroughNodePin m_PinDepthStencil;

  ezMat4 m_lastViewProjection;  // Not stereo aware.
  ezGALBufferHandle m_randomNumberBuffer;
  ezDynamicArray<float> m_randomNumbers;
  ezRandom m_randomGenerator;
};

#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// \brief A very basic render pass that renders into the color target.
///
/// Can either works as passthrough or if no input is present creates
/// output targets matching the view's render target.
/// Needs to be connected to a ezTargetPass to function.
class EZ_RENDERERCORE_DLL ezSimpleRenderPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSimpleRenderPass, ezRenderPipelinePass);

public:
  ezSimpleRenderPass(const char* szName = "SimpleRenderPass");
  ~ezSimpleRenderPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

  void SetMessage(const char* szMessage);

protected:
  ezPassThroughNodePin m_PinColor;
  ezPassThroughNodePin m_PinDepthStencil;
  ezInputNodePin m_PinResolvedDepth;

  ezString m_sMessage;
};


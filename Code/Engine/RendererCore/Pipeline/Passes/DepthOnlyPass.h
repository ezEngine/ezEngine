#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// \brief A render pass that renders into a depth target only.
class EZ_RENDERERCORE_DLL ezDepthOnlyPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDepthOnlyPass, ezRenderPipelinePass);

public:
  ezDepthOnlyPass(const char* szName = "DepthOnlyPass");
  ~ezDepthOnlyPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodePassThroughPin m_PinDepthStencil;

  bool m_bRenderStaticObjects = true;
  bool m_bRenderDynamicObjects = true;
  bool m_bRenderTransparentObjects = false;
};

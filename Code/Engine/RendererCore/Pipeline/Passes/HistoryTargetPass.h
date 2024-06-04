#pragma once

#include <RendererCore/Pipeline/Passes/HistorySourcePass.h>

/// \brief Allows to write data to be accessible in the next frame. See ezHistorySourcePass for usage.
class EZ_RENDERERCORE_DLL ezHistoryTargetPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezHistoryTargetPass, ezRenderPipelinePass);

public:
  ezHistoryTargetPass(const char* szName = "HistoryTargetPass");
  ~ezHistoryTargetPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual ezGALTextureHandle QueryTextureProvider(const ezRenderPipelineNodePin* pPin, const ezGALTextureCreationDescription& desc) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodeInputProviderPin m_PinInput;
  ezString m_sSourcePassName;
};

#pragma once

#include <RendererCore/Pipeline/Passes/HistorySourcePass.h>

/// Render pass that stores data to be accessible in the next frame.
///
/// Works in pairs with ezHistorySourcePass. Receives the current frame's data as input
/// and stores it for the next frame. Set SourcePassName to match the corresponding
/// ezHistorySourcePass. See ezHistorySourcePass for usage details.
class EZ_RENDERERCORE_DLL ezHistoryTargetPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezHistoryTargetPass, ezRenderPipelinePass);

public:
  ezHistoryTargetPass(const char* szName = "HistoryTargetPass");
  ~ezHistoryTargetPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  /// Provides the history texture handle for the input pin.
  virtual ezGALTextureHandle QueryTextureProvider(const ezRenderPipelineNodePin* pPin, const ezGALTextureCreationDescription& desc) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodeInputProviderPin m_PinInput;  ///< Input texture to store for next frame.
  ezString m_sSourcePassName = "HistorySourcePass"; ///< Name of the paired ezHistorySourcePass.
};

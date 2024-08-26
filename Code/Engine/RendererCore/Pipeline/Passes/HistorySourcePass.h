#pragma once

#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Pipeline/Passes/SourcePass.h>

/// \brief Allows to access data from a previous frame. Always comes in a pair with a ezHistoryTargetPass.
/// To preserve textures across the next frame you need to create this node to define the type of texture and initial state. This node's output pin will give access to the previous frame's content.
/// Next, create an ezHistoryTargetPass. It's input pin exposes the same texture as provided by the source node but allows you to write to by connecting the input pin to another pass that produces the image that you want to carry to the next frame. To connect an ezHistoryTargetPass to its counterpart you need to set it's "SourcePassName" property to the name of the ezHistorySourcePass you want to match.
/// As both nodes expose the same texture, special care has to be taken that it's not used as input and output of another pass at the same time. In those cases, add a ezCopyTexturePass to break up invalid state.
class EZ_RENDERERCORE_DLL ezHistorySourcePass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezHistorySourcePass, ezRenderPipelinePass);

public:
  ezHistorySourcePass(const char* szName = "HistorySourcePass");
  ~ezHistorySourcePass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual ezGALTextureHandle QueryTextureProvider(const ezRenderPipelineNodePin* pPin, const ezGALTextureCreationDescription& desc) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodeOutputProviderPin m_PinOutput;
  ezEnum<ezSourceFormat> m_Format = ezSourceFormat::Default;
  ezEnum<ezGALMSAASampleCount> m_MsaaMode = ezGALMSAASampleCount::None;
  ezColor m_ClearColor = ezColor::Black;

  bool m_bFirstExecute = true;
};

class EZ_RENDERERCORE_DLL ezHistorySourcePassTextureDataProvider : public ezFrameDataProviderBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezHistorySourcePassTextureDataProvider, ezFrameDataProviderBase);

public:
  ezHistorySourcePassTextureDataProvider();
  ~ezHistorySourcePassTextureDataProvider();

  void ResetTexture(ezStringView sSourcePassName);
  ezGALTextureHandle GetOrCreateTexture(ezStringView sSourcePassName, const ezGALTextureCreationDescription& desc);

public:
  ezHashTable<ezString, ezGALTextureHandle> m_Data;

private:
  // We ignore the frame-based logic for this data provider as we only want to store cross frame data.
  virtual void* UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData) override { return nullptr; }
};

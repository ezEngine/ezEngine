#pragma once

#include "RendererCore/Pipeline/FrameDataProvider.h"
#include <RendererCore/Pipeline/RenderPipelinePass.h>

struct ezSourceFormat
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Color4Channel8BitNormalized_sRGB,
    Color4Channel8BitNormalized,
    Color4Channel16BitFloat,
    Color4Channel32BitFloat,
    Color3Channel11_11_10BitFloat,
    Depth16Bit,
    Depth24BitStencil8Bit,
    Depth32BitFloat,

    Default = Color4Channel8BitNormalized_sRGB
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezSourceFormat);

class EZ_RENDERERCORE_DLL ezSourcePass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSourcePass, ezRenderPipelinePass);

public:
  ezSourcePass(const char* szName = "SourcePass");
  ~ezSourcePass();

  static ezGALTextureCreationDescription GetOutputDescription(const ezView& view, ezEnum<ezSourceFormat> format, ezEnum<ezGALMSAASampleCount> msaaMode);
  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodeOutputPin m_PinOutput;

  ezEnum<ezSourceFormat> m_Format;
  ezEnum<ezGALMSAASampleCount> m_MsaaMode;
  ezColor m_ClearColor;
  bool m_bClear = false;
};

#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// Supported texture formats for source render targets.
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

/// Render pass that creates a render target for other passes to render into.
///
/// Entry point pass that allocates and optionally clears a render target with specified
/// format and MSAA settings. The output is then used by downstream passes for rendering.
class EZ_RENDERERCORE_DLL ezSourcePass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSourcePass, ezRenderPipelinePass);

public:
  ezSourcePass(const char* szName = "SourcePass");
  ~ezSourcePass();

  /// Builds a texture description for the output based on format and MSAA settings.
  static ezGALTextureCreationDescription GetOutputDescription(const ezView& view, ezEnum<ezSourceFormat> format, ezEnum<ezGALMSAASampleCount> msaaMode);
  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;
  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodeOutputPin m_PinOutput;                            ///< Output render target.

  ezEnum<ezSourceFormat> m_Format = ezSourceFormat::Default;            ///< Texture format for the render target.
  ezEnum<ezGALMSAASampleCount> m_MsaaMode = ezGALMSAASampleCount::None; ///< MSAA sample count.
  ezColor m_ClearColor = ezColor::Black;                                ///< Clear color if clearing is enabled.
  bool m_bClear = false;                                                ///< Whether to clear the render target on each execution.
};

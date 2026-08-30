#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// Minimum number of bits per channel that a texture format has to provide.
///
/// The values are the bit counts themselves, so a larger value always means a higher precision. Format selection picks the cheapest format that provides at least this precision, so a value that no format satisfies results in an error rather than a silent downgrade.
struct ezRequiredTexturePrecision
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Bits_5 = 5,
    Bits_8 = 8,
    Bits_10 = 10,
    Bits_16 = 16,
    Bits_24 = 24,
    Bits_32 = 32,
    Default = Bits_8
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezRequiredTexturePrecision);

/// How the contents of a texture are interpreted by the GPU.
///
/// Together with ezRequiredTexturePrecision and ezRequiredTextureChannels this describes what a pass needs from a texture, instead of naming a concrete ezGALResourceFormat that may not exist on every device.
struct ezRequiredTextureType
{
  using StorageType = ezUInt8;

  enum Enum
  {
    UNorm = 1,
    SNorm = 2,
    SRGB = 3,
    UInt = 4,
    SInt = 5,
    Float = 6,
    Depth = 7, ///< Depth formats use the channel count to request a stencil part: Channels_1 is depth only, Channels_2 is depth and stencil.
    Default = SRGB
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezRequiredTextureType);

/// Minimum number of channels that a texture format has to provide.
///
/// Format selection may pick a format with more channels if no exact match is supported by the device.
struct ezRequiredTextureChannels
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Channels_1 = 1,
    Channels_2 = 2,
    Channels_3 = 3,
    Channels_4 = 4,
    Default = Channels_4
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezRequiredTextureChannels);

class ezAbstractObjectNode;

/// Maps a texture format that was serialized before the switch to ezRequiredTexture* onto the new requirements.
///
/// \param uiLegacyValue The raw integer value that was stored in the old data.
/// \param bGalResourceFormat If true, the value is an ezGALResourceFormat, otherwise it is the removed ezSourceFormat enum.
EZ_RENDERERCORE_DLL void ezGetLegacyTextureFormatRequirements(ezUInt32 uiLegacyValue, bool bGalResourceFormat, ezEnum<ezRequiredTextureType>& out_type, ezEnum<ezRequiredTexturePrecision>& out_precision, ezEnum<ezRequiredTextureChannels>& out_channels);

/// Replaces the 'Format' property of an older graph node with the Type / Precision / Channels properties that describe the same texture.
/// \param bGalResourceFormat If true, the property holds an ezGALResourceFormat name, otherwise a name of the removed ezSourceFormat enum.
EZ_RENDERERCORE_DLL void ezPatchLegacyTextureFormatProperty(ezAbstractObjectNode* pNode, bool bGalResourceFormat);

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

  /// Picks the cheapest ezGALResourceFormat that satisfies all requirements and is supported by the current device.
  ///
  /// \return ezGALResourceFormat::Invalid if no supported format matches.
  static ezEnum<ezGALResourceFormat> FindFormat(ezEnum<ezRequiredTextureType> type, ezEnum<ezRequiredTexturePrecision> minPrecision, ezEnum<ezRequiredTextureChannels> minChannels, ezBitflags<ezGALResourceFormatSupport> requiredSupport);

  /// Builds a render target description that matches the given requirements, the view's viewport size and the camera's stereo mode.
  ///
  /// Fails if the device supports no format for the requested combination.
  static ezStatus GetOutputDescription(const ezViewData& viewData, const ezCamera& camera, ezEnum<ezRequiredTextureType> type, ezEnum<ezRequiredTexturePrecision> minPrecision, ezEnum<ezRequiredTextureChannels> minChannels, ezEnum<ezGALMSAASampleCount> msaaMode, bool bUAV, ezGALTextureCreationDescription& out_desc);

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  ezRenderPipelineNodeOutputPin m_PinOutput;                                               ///< Output render target.

  ezEnum<ezRequiredTextureType> m_Type = ezRequiredTextureType::SRGB;                      ///< How the texture contents are interpreted.
  ezEnum<ezRequiredTexturePrecision> m_MinPrecision = ezRequiredTexturePrecision::Bits_8;  ///< Minimum bits per channel.
  ezEnum<ezRequiredTextureChannels> m_MinChannels = ezRequiredTextureChannels::Channels_4; ///< Minimum channel count.
  ezEnum<ezGALMSAASampleCount> m_MsaaMode = ezGALMSAASampleCount::None;                    ///< MSAA sample count.
  ezColor m_ClearColor = ezColor::Black;                                                   ///< Clear color if clearing is enabled and the format is not a depth format.
  float m_fClearDepth = 1.0f;                                                              ///< Clear depth if clearing is enabled and the format is a depth format.
  bool m_bClear = false;                                                                   ///< Whether to clear the render target on each execution.
  bool m_bUAV = false;                                                                     ///< Whether the texture also has to be writable from compute shaders.
};

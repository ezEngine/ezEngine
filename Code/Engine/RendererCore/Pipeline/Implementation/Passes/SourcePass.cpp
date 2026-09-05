#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/SourcePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSourcePass, 4, ezRTTIDefaultAllocator<ezSourcePass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_ENUM_MEMBER_PROPERTY("Type", ezRequiredTextureType, m_Type),
    EZ_ENUM_MEMBER_PROPERTY("Precision", ezRequiredTexturePrecision, m_MinPrecision),
    EZ_ENUM_MEMBER_PROPERTY("Channels", ezRequiredTextureChannels, m_MinChannels),
    EZ_ENUM_MEMBER_PROPERTY("MSAA_Mode", ezGALMSAASampleCount, m_MsaaMode),
    EZ_MEMBER_PROPERTY("UAV", m_bUAV),
    EZ_MEMBER_PROPERTY("ClearColor", m_ClearColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_MEMBER_PROPERTY("ClearDepth", m_fClearDepth)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("Clear", m_bClear),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Input")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezRequiredTexturePrecision, 1)
  EZ_ENUM_CONSTANTS(
    ezRequiredTexturePrecision::Bits_5,
    ezRequiredTexturePrecision::Bits_8,
    ezRequiredTexturePrecision::Bits_10,
    ezRequiredTexturePrecision::Bits_16,
    ezRequiredTexturePrecision::Bits_24,
    ezRequiredTexturePrecision::Bits_32)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezRequiredTextureType, 1)
  EZ_ENUM_CONSTANTS(
    ezRequiredTextureType::UNorm,
    ezRequiredTextureType::SNorm,
    ezRequiredTextureType::SRGB,
    ezRequiredTextureType::UInt,
    ezRequiredTextureType::SInt,
    ezRequiredTextureType::Float,
    ezRequiredTextureType::Depth)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezRequiredTextureChannels, 1)
  EZ_ENUM_CONSTANTS(
    ezRequiredTextureChannels::Channels_1,
    ezRequiredTextureChannels::Channels_2,
    ezRequiredTextureChannels::Channels_3,
    ezRequiredTextureChannels::Channels_4)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezSourcePass::ezSourcePass(const char* szName)
  : ezRenderPipelinePass(szName, true)
{
}

ezSourcePass::~ezSourcePass() = default;

namespace
{
  /// The removed ezSourceFormat enum. Only kept around to be able to read data that was written before texture requirements replaced it.
  enum class LegacySourceFormat : ezUInt32
  {
    Color4Channel8BitNormalized_sRGB = 0,
    Color4Channel8BitNormalized = 1,
    Color4Channel16BitFloat = 2,
    Color4Channel32BitFloat = 3,
    Color3Channel11_11_10BitFloat = 4,
    Depth16Bit = 5,
    Depth24BitStencil8Bit = 6,
    Depth32BitFloat = 7,
  };

  struct TextureFormat
  {
    ezRequiredTextureChannels::Enum m_Channels;
    ezRequiredTexturePrecision::Enum m_Precision;
    ezGALResourceFormat::Enum m_Format;
  };

  static ezBitflags<ezGALResourceFormatSupport> GetRequiredFormatSupport(ezEnum<ezGALMSAASampleCount> msaaMode, bool bUAV)
  {
    ezBitflags<ezGALResourceFormatSupport> support = ezGALResourceFormatSupport::RenderTarget | ezGALResourceFormatSupport::Texture;
    if (bUAV)
      support.Add(ezGALResourceFormatSupport::TextureRW);

    switch (msaaMode)
    {
      case ezGALMSAASampleCount::TwoSamples:
        support.Add(ezGALResourceFormatSupport::MSAA2x);
        break;
      case ezGALMSAASampleCount::FourSamples:
        support.Add(ezGALResourceFormatSupport::MSAA4x);
        break;
      case ezGALMSAASampleCount::EightSamples:
        support.Add(ezGALResourceFormatSupport::MSAA8x);
        break;
      default:
        break;
    }
    return support;
  }

  static void GetLegacyFormatRequirements(LegacySourceFormat format, ezEnum<ezRequiredTextureType>& out_type, ezEnum<ezRequiredTexturePrecision>& out_precision, ezEnum<ezRequiredTextureChannels>& out_channels)
  {
    out_channels = ezRequiredTextureChannels::Channels_4;
    switch (format)
    {
      case LegacySourceFormat::Color4Channel8BitNormalized_sRGB:
        out_type = ezRequiredTextureType::SRGB;
        out_precision = ezRequiredTexturePrecision::Bits_8;
        break;
      case LegacySourceFormat::Color4Channel8BitNormalized:
        out_type = ezRequiredTextureType::UNorm;
        out_precision = ezRequiredTexturePrecision::Bits_8;
        break;
      case LegacySourceFormat::Color4Channel16BitFloat:
        out_type = ezRequiredTextureType::Float;
        out_precision = ezRequiredTexturePrecision::Bits_16;
        break;
      case LegacySourceFormat::Color4Channel32BitFloat:
        out_type = ezRequiredTextureType::Float;
        out_precision = ezRequiredTexturePrecision::Bits_32;
        break;
      case LegacySourceFormat::Color3Channel11_11_10BitFloat:
        out_type = ezRequiredTextureType::Float;
        out_precision = ezRequiredTexturePrecision::Bits_10;
        out_channels = ezRequiredTextureChannels::Channels_3;
        break;
      case LegacySourceFormat::Depth16Bit:
        out_type = ezRequiredTextureType::Depth;
        out_precision = ezRequiredTexturePrecision::Bits_16;
        out_channels = ezRequiredTextureChannels::Channels_1;
        break;
      case LegacySourceFormat::Depth24BitStencil8Bit:
        out_type = ezRequiredTextureType::Depth;
        out_precision = ezRequiredTexturePrecision::Bits_24;
        out_channels = ezRequiredTextureChannels::Channels_2;
        break;
      case LegacySourceFormat::Depth32BitFloat:
        out_type = ezRequiredTextureType::Depth;
        out_precision = ezRequiredTexturePrecision::Bits_32;
        out_channels = ezRequiredTextureChannels::Channels_1;
        break;
      default:
        out_type = ezRequiredTextureType::Default;
        out_precision = ezRequiredTexturePrecision::Default;
        out_channels = ezRequiredTextureChannels::Default;
        break;
    }
  }

  static LegacySourceFormat GetLegacySourceFormat(ezGALResourceFormat::Enum format)
  {
    switch (format)
    {
      case ezGALResourceFormat::RGBAHalf:
        return LegacySourceFormat::Color4Channel16BitFloat;
      case ezGALResourceFormat::RGBAFloat:
        return LegacySourceFormat::Color4Channel32BitFloat;
      case ezGALResourceFormat::RG11B10Float:
        return LegacySourceFormat::Color3Channel11_11_10BitFloat;
      case ezGALResourceFormat::D16:
        return LegacySourceFormat::Depth16Bit;
      case ezGALResourceFormat::D24S8:
        return LegacySourceFormat::Depth24BitStencil8Bit;
      case ezGALResourceFormat::DFloat:
        return LegacySourceFormat::Depth32BitFloat;
      case ezGALResourceFormat::RGBAUByteNormalized:
      case ezGALResourceFormat::BGRAUByteNormalized:
        return LegacySourceFormat::Color4Channel8BitNormalized;
      default:
        return LegacySourceFormat::Color4Channel8BitNormalized_sRGB;
    }
  }
} // namespace

void ezGetLegacyTextureFormatRequirements(ezUInt32 uiLegacyValue, bool bGalResourceFormat, ezEnum<ezRequiredTextureType>& out_type, ezEnum<ezRequiredTexturePrecision>& out_precision, ezEnum<ezRequiredTextureChannels>& out_channels)
{
  const LegacySourceFormat format = bGalResourceFormat ? GetLegacySourceFormat(static_cast<ezGALResourceFormat::Enum>(uiLegacyValue)) : static_cast<LegacySourceFormat>(uiLegacyValue);
  GetLegacyFormatRequirements(format, out_type, out_precision, out_channels);
}

ezEnum<ezGALResourceFormat> ezSourcePass::FindFormat(ezEnum<ezRequiredTextureType> type, ezEnum<ezRequiredTexturePrecision> minPrecision, ezEnum<ezRequiredTextureChannels> minChannels, ezBitflags<ezGALResourceFormatSupport> requiredSupport)
{
  static constexpr TextureFormat unormTextures[] = {
    {ezRequiredTextureChannels::Channels_1, ezRequiredTexturePrecision::Bits_8, ezGALResourceFormat::RUByteNormalized},
    {ezRequiredTextureChannels::Channels_1, ezRequiredTexturePrecision::Bits_16, ezGALResourceFormat::RUShortNormalized},
    {ezRequiredTextureChannels::Channels_2, ezRequiredTexturePrecision::Bits_8, ezGALResourceFormat::RGUByteNormalized},
    {ezRequiredTextureChannels::Channels_2, ezRequiredTexturePrecision::Bits_16, ezGALResourceFormat::RGUShortNormalized},
    {ezRequiredTextureChannels::Channels_3, ezRequiredTexturePrecision::Bits_5, ezGALResourceFormat::B5G6R5UNormalized},
    {ezRequiredTextureChannels::Channels_3, ezRequiredTexturePrecision::Bits_10, ezGALResourceFormat::RGB10A2UIntNormalized},
    {ezRequiredTextureChannels::Channels_4, ezRequiredTexturePrecision::Bits_8, ezGALResourceFormat::BGRAUByteNormalized},
    {ezRequiredTextureChannels::Channels_4, ezRequiredTexturePrecision::Bits_8, ezGALResourceFormat::RGBAUByteNormalized},
    {ezRequiredTextureChannels::Channels_4, ezRequiredTexturePrecision::Bits_16, ezGALResourceFormat::RGBAUShortNormalized}};
  static constexpr TextureFormat snormTextures[] = {
    {ezRequiredTextureChannels::Channels_1, ezRequiredTexturePrecision::Bits_8, ezGALResourceFormat::RByteNormalized},
    {ezRequiredTextureChannels::Channels_1, ezRequiredTexturePrecision::Bits_16, ezGALResourceFormat::RShortNormalized},
    {ezRequiredTextureChannels::Channels_2, ezRequiredTexturePrecision::Bits_8, ezGALResourceFormat::RGByteNormalized},
    {ezRequiredTextureChannels::Channels_2, ezRequiredTexturePrecision::Bits_16, ezGALResourceFormat::RGShortNormalized},
    {ezRequiredTextureChannels::Channels_4, ezRequiredTexturePrecision::Bits_8, ezGALResourceFormat::RGBAByteNormalized},
    {ezRequiredTextureChannels::Channels_4, ezRequiredTexturePrecision::Bits_16, ezGALResourceFormat::RGBAShortNormalized}};
  static constexpr TextureFormat srgbTextures[] = {
    {ezRequiredTextureChannels::Channels_4, ezRequiredTexturePrecision::Bits_8, ezGALResourceFormat::BGRAUByteNormalizedsRGB},
    {ezRequiredTextureChannels::Channels_4, ezRequiredTexturePrecision::Bits_8, ezGALResourceFormat::RGBAUByteNormalizedsRGB}};
  static constexpr TextureFormat uintTextures[] = {
    {ezRequiredTextureChannels::Channels_1, ezRequiredTexturePrecision::Bits_8, ezGALResourceFormat::RUByte},
    {ezRequiredTextureChannels::Channels_1, ezRequiredTexturePrecision::Bits_16, ezGALResourceFormat::RUShort},
    {ezRequiredTextureChannels::Channels_1, ezRequiredTexturePrecision::Bits_32, ezGALResourceFormat::RUInt},
    {ezRequiredTextureChannels::Channels_2, ezRequiredTexturePrecision::Bits_8, ezGALResourceFormat::RGUByte},
    {ezRequiredTextureChannels::Channels_2, ezRequiredTexturePrecision::Bits_16, ezGALResourceFormat::RGUShort},
    {ezRequiredTextureChannels::Channels_2, ezRequiredTexturePrecision::Bits_32, ezGALResourceFormat::RGUInt},
    {ezRequiredTextureChannels::Channels_3, ezRequiredTexturePrecision::Bits_10, ezGALResourceFormat::RGB10A2UInt},
    {ezRequiredTextureChannels::Channels_3, ezRequiredTexturePrecision::Bits_32, ezGALResourceFormat::RGBUInt},
    {ezRequiredTextureChannels::Channels_4, ezRequiredTexturePrecision::Bits_8, ezGALResourceFormat::RGBAUByte},
    {ezRequiredTextureChannels::Channels_4, ezRequiredTexturePrecision::Bits_16, ezGALResourceFormat::RGBAUShort},
    {ezRequiredTextureChannels::Channels_4, ezRequiredTexturePrecision::Bits_32, ezGALResourceFormat::RGBAUInt}};
  static constexpr TextureFormat sintTextures[] = {
    {ezRequiredTextureChannels::Channels_1, ezRequiredTexturePrecision::Bits_8, ezGALResourceFormat::RByte},
    {ezRequiredTextureChannels::Channels_1, ezRequiredTexturePrecision::Bits_16, ezGALResourceFormat::RShort},
    {ezRequiredTextureChannels::Channels_1, ezRequiredTexturePrecision::Bits_32, ezGALResourceFormat::RInt},
    {ezRequiredTextureChannels::Channels_2, ezRequiredTexturePrecision::Bits_8, ezGALResourceFormat::RGByte},
    {ezRequiredTextureChannels::Channels_2, ezRequiredTexturePrecision::Bits_16, ezGALResourceFormat::RGShort},
    {ezRequiredTextureChannels::Channels_2, ezRequiredTexturePrecision::Bits_32, ezGALResourceFormat::RGInt},
    {ezRequiredTextureChannels::Channels_3, ezRequiredTexturePrecision::Bits_32, ezGALResourceFormat::RGBInt},
    {ezRequiredTextureChannels::Channels_4, ezRequiredTexturePrecision::Bits_8, ezGALResourceFormat::RGBAByte},
    {ezRequiredTextureChannels::Channels_4, ezRequiredTexturePrecision::Bits_16, ezGALResourceFormat::RGBAShort},
    {ezRequiredTextureChannels::Channels_4, ezRequiredTexturePrecision::Bits_32, ezGALResourceFormat::RGBAInt}};
  static constexpr TextureFormat floatTextures[] = {
    {ezRequiredTextureChannels::Channels_1, ezRequiredTexturePrecision::Bits_16, ezGALResourceFormat::RHalf},
    {ezRequiredTextureChannels::Channels_1, ezRequiredTexturePrecision::Bits_32, ezGALResourceFormat::RFloat},
    {ezRequiredTextureChannels::Channels_2, ezRequiredTexturePrecision::Bits_16, ezGALResourceFormat::RGHalf},
    {ezRequiredTextureChannels::Channels_2, ezRequiredTexturePrecision::Bits_32, ezGALResourceFormat::RGFloat},
    {ezRequiredTextureChannels::Channels_3, ezRequiredTexturePrecision::Bits_10, ezGALResourceFormat::RG11B10Float},
    {ezRequiredTextureChannels::Channels_3, ezRequiredTexturePrecision::Bits_32, ezGALResourceFormat::RGBFloat},
    {ezRequiredTextureChannels::Channels_4, ezRequiredTexturePrecision::Bits_16, ezGALResourceFormat::RGBAHalf},
    {ezRequiredTextureChannels::Channels_4, ezRequiredTexturePrecision::Bits_32, ezGALResourceFormat::RGBAFloat}};
  static constexpr TextureFormat depthTextures[] = {
    {ezRequiredTextureChannels::Channels_1, ezRequiredTexturePrecision::Bits_16, ezGALResourceFormat::D16},
    {ezRequiredTextureChannels::Channels_1, ezRequiredTexturePrecision::Bits_32, ezGALResourceFormat::DFloat},
    {ezRequiredTextureChannels::Channels_2, ezRequiredTexturePrecision::Bits_24, ezGALResourceFormat::D24S8}};

  ezArrayPtr<const TextureFormat> formats;
  switch (type)
  {
    case ezRequiredTextureType::UNorm:
      formats = unormTextures;
      break;
    case ezRequiredTextureType::SNorm:
      formats = snormTextures;
      break;
    case ezRequiredTextureType::SRGB:
      formats = srgbTextures;
      break;
    case ezRequiredTextureType::UInt:
      formats = uintTextures;
      break;
    case ezRequiredTextureType::SInt:
      formats = sintTextures;
      break;
    case ezRequiredTextureType::Float:
      formats = floatTextures;
      break;
    case ezRequiredTextureType::Depth:
      formats = depthTextures;
      break;
    default:
      return ezGALResourceFormat::Invalid;
  }

  const ezGALDeviceCapabilities& caps = ezGALDevice::GetDefaultDevice()->GetCapabilities();
  for (const TextureFormat& candidate : formats)
  {
    if (candidate.m_Channels >= minChannels && candidate.m_Precision >= minPrecision && caps.m_FormatSupport[candidate.m_Format].AreAllSet(requiredSupport))
      return candidate.m_Format;
  }
  return ezGALResourceFormat::Invalid;
}

ezStatus ezSourcePass::GetOutputDescription(const ezViewData& viewData, const ezCamera& camera, ezEnum<ezRequiredTextureType> type, ezEnum<ezRequiredTexturePrecision> minPrecision, ezEnum<ezRequiredTextureChannels> minChannels, ezEnum<ezGALMSAASampleCount> msaaMode, bool bUAV, ezGALTextureCreationDescription& out_desc)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  const ezBitflags<ezGALResourceFormatSupport> requiredSupport = GetRequiredFormatSupport(msaaMode, bUAV);
  ezEnum<ezGALResourceFormat> format = FindFormat(type, minPrecision, minChannels, requiredSupport);
  if (format == ezGALResourceFormat::Invalid)
  {
    return ezStatus(ezFmt("No texture format found for type '{}', precision '{}', channels '{}', MSAA '{}', UAV '{}'.", ezArgEnum(type), ezArgEnum(minPrecision), ezArgEnum(minChannels), ezArgEnum(msaaMode), bUAV));
  }

  // Match the active render target's channel order when it satisfies the exact common backbuffer requirements.
  if (minChannels == ezRequiredTextureChannels::Channels_4 && minPrecision == ezRequiredTexturePrecision::Bits_8 &&
      (type == ezRequiredTextureType::UNorm || type == ezRequiredTextureType::SRGB))
  {
    const ezGALRenderTargets& renderTargets = viewData.GetActiveRenderTargets();
    if (const ezGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]))
    {
      const ezGALTextureCreationDescription& renderTargetDesc = pTexture->GetDescription();
      const ezGALResourceFormat::Enum preferredFormat = renderTargetDesc.m_Format;
      const bool bMatchingType = (type == ezRequiredTextureType::SRGB) == ezGALResourceFormat::IsSrgb(preferredFormat);
      if (bMatchingType && !ezGALResourceFormat::IsIntegerFormat(preferredFormat) && !ezGALResourceFormat::IsDepthFormat(preferredFormat) &&
          ezGALResourceFormat::GetChannelCount(preferredFormat) == 4 && ezGALResourceFormat::GetBitsPerElement(preferredFormat) == 32 && pDevice->GetCapabilities().m_FormatSupport[preferredFormat].AreAllSet(requiredSupport))
      {
        format = preferredFormat;
      }
    }
  }

  out_desc.SetAsRenderTarget(static_cast<ezUInt32>(viewData.m_ViewPortRect.width), static_cast<ezUInt32>(viewData.m_ViewPortRect.height), camera.IsStereoscopic() ? 2 : 1, format, msaaMode);
  out_desc.m_Type = ezGALTextureType::Texture2DArray;
  if (bUAV)
    out_desc.m_TextureFlags.Add(ezGALTextureUsageFlags::UnorderedAccess);

  return EZ_SUCCESS;
}

ezStatus ezSourcePass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezGALTextureCreationDescription desc;
  EZ_SUCCEED_OR_RETURN(GetOutputDescription(viewData, camera, m_Type, m_MinPrecision, m_MinChannels, m_MsaaMode, m_bUAV, desc));
  ezRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(desc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  if (m_bClear)
  {
    if (ezGALResourceFormat::IsDepthFormat(desc.m_Format))
    {
      auto pass = ref_graph.AddGraphicsPass("ClearDepth");
      pass.AddDepthStencilTarget(hOutput, {}, ezGALRenderTargetLoadOp::Clear, {}, ezGALRenderTargetLoadOp::Clear);
      pass.SetClearDepth(m_fClearDepth);
      pass.SetClearStencil();
    }
    else
    {
      auto pass = ref_graph.AddGraphicsPass("ClearColor");
      pass.AddColorTarget(hOutput, {}, ezGALRenderTargetLoadOp::Clear);
      pass.SetClearColor(0, m_ClearColor);
    }
  }

  return EZ_SUCCESS;
}

ezResult ezSourcePass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_Type;
  inout_stream << m_MinPrecision;
  inout_stream << m_MinChannels;
  inout_stream << m_MsaaMode;
  inout_stream << m_ClearColor;
  inout_stream << m_fClearDepth;
  inout_stream << m_bClear;
  inout_stream << m_bUAV;
  return EZ_SUCCESS;
}

ezResult ezSourcePass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  if (uiVersion >= 4)
  {
    inout_stream >> m_Type;
    inout_stream >> m_MinPrecision;
    inout_stream >> m_MinChannels;
    inout_stream >> m_MsaaMode;
    inout_stream >> m_ClearColor;
    inout_stream >> m_fClearDepth;
    inout_stream >> m_bClear;
    inout_stream >> m_bUAV;
  }
  else
  {
    // Version 3 stored the removed ezSourceFormat, everything before that an ezGALResourceFormat. Both use ezUInt8 as storage.
    ezUInt8 uiLegacyFormat = 0;
    inout_stream >> uiLegacyFormat;
    ezGetLegacyTextureFormatRequirements(uiLegacyFormat, uiVersion < 3, m_Type, m_MinPrecision, m_MinChannels);

    inout_stream >> m_MsaaMode;
    inout_stream >> m_ClearColor;
    inout_stream >> m_bClear;
  }
  return EZ_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

namespace
{
  /// Resolves a name of the removed ezSourceFormat enum back to its former integer value.
  static ezUInt32 GetLegacySourceFormatValue(ezStringView sName)
  {
    static constexpr ezStringView names[] = {
      "Color4Channel8BitNormalized_sRGB"_ezsv,
      "Color4Channel8BitNormalized"_ezsv,
      "Color4Channel16BitFloat"_ezsv,
      "Color4Channel32BitFloat"_ezsv,
      "Color3Channel11_11_10BitFloat"_ezsv,
      "Depth16Bit"_ezsv,
      "Depth24BitStencil8Bit"_ezsv,
      "Depth32BitFloat"_ezsv,
    };

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(names); ++i)
    {
      if (sName.EndsWith(names[i]))
        return i;
    }

    return static_cast<ezUInt32>(LegacySourceFormat::Color4Channel8BitNormalized_sRGB);
  }
} // namespace

void ezPatchLegacyTextureFormatProperty(ezAbstractObjectNode* pNode, bool bGalResourceFormat)
{
  const ezAbstractObjectNode::Property* pFormat = pNode->FindProperty("Format");
  if (pFormat == nullptr)
    return;

  const ezString sFormat = pFormat->m_Value.ConvertTo<ezString>();
  ezUInt32 uiLegacyValue = 0;
  if (bGalResourceFormat)
  {
    ezEnum<ezGALResourceFormat> galFormat;
    ezReflectionUtils::StringToEnumeration<ezGALResourceFormat>(sFormat.GetData(), galFormat);
    uiLegacyValue = galFormat.GetValue();
  }
  else
  {
    uiLegacyValue = GetLegacySourceFormatValue(sFormat);
  }

  ezEnum<ezRequiredTextureType> type;
  ezEnum<ezRequiredTexturePrecision> precision;
  ezEnum<ezRequiredTextureChannels> channels;
  ezGetLegacyTextureFormatRequirements(uiLegacyValue, bGalResourceFormat, type, precision, channels);

  ezStringBuilder sValue;
  ezReflectionUtils::EnumerationToString(type, sValue);
  pNode->AddProperty("Type", sValue.GetView());
  ezReflectionUtils::EnumerationToString(precision, sValue);
  pNode->AddProperty("Precision", sValue.GetView());
  ezReflectionUtils::EnumerationToString(channels, sValue);
  pNode->AddProperty("Channels", sValue.GetView());
  pNode->RemoveProperty("Format");
}

class ezSourcePassPatch_1_2 : public ezGraphPatch
{
public:
  ezSourcePassPatch_1_2()
    : ezGraphPatch("ezSourcePass", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("MSAA Mode", "MSAA_Mode");
    pNode->RenameProperty("Clear Color", "ClearColor");
  }
};

ezSourcePassPatch_1_2 g_ezSourcePassPatch_1_2;

class ezSourcePassPatch_2_3 : public ezGraphPatch
{
public:
  ezSourcePassPatch_2_3()
    : ezGraphPatch("ezSourcePass", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    // will actually patch from version 2 to 4 schema, the call in ezSourcePassPatch_3_4 will be a no-op.
    ezPatchLegacyTextureFormatProperty(pNode, true);
  }
};

ezSourcePassPatch_2_3 g_ezSourcePassPatch_2_3;

class ezSourcePassPatch_3_4 : public ezGraphPatch
{
public:
  ezSourcePassPatch_3_4()
    : ezGraphPatch("ezSourcePass", 4)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    ezPatchLegacyTextureFormatProperty(pNode, false);
  }
};

ezSourcePassPatch_3_4 g_ezSourcePassPatch_3_4;

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SourcePass);

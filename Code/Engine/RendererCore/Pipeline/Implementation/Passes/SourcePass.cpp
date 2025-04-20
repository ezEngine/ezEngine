#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/SourcePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSourcePass, 3, ezRTTIDefaultAllocator<ezSourcePass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_ENUM_MEMBER_PROPERTY("Format", ezSourceFormat, m_Format),
    EZ_ENUM_MEMBER_PROPERTY("MSAA_Mode", ezGALMSAASampleCount, m_MsaaMode),
    EZ_MEMBER_PROPERTY("ClearColor", m_ClearColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
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

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSourceFormat, 1)
  EZ_ENUM_CONSTANTS(
    ezSourceFormat::Color4Channel8BitNormalized_sRGB,
    ezSourceFormat::Color4Channel8BitNormalized,
    ezSourceFormat::Color4Channel16BitFloat,
    ezSourceFormat::Color4Channel32BitFloat,
    ezSourceFormat::Color3Channel11_11_10BitFloat,
    ezSourceFormat::Depth16Bit,
    ezSourceFormat::Depth24BitStencil8Bit,
    ezSourceFormat::Depth32BitFloat
  )
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezSourcePass::ezSourcePass(const char* szName)
  : ezRenderPipelinePass(szName, true)
{
}

ezSourcePass::~ezSourcePass() = default;

ezGALTextureCreationDescription ezSourcePass::GetOutputDescription(const ezView& view, ezEnum<ezSourceFormat> format, ezEnum<ezGALMSAASampleCount> msaaMode)
{
  ezUInt32 uiWidth = static_cast<ezUInt32>(view.GetViewport().width);
  ezUInt32 uiHeight = static_cast<ezUInt32>(view.GetViewport().height);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  const ezGALRenderTargets& renderTargets = view.GetActiveRenderTargets();

  ezGALTextureCreationDescription desc;
  desc.m_Type = ezGALTextureType::Texture2DArray;

  // Color
  if (format == ezSourceFormat::Color4Channel8BitNormalized || format == ezSourceFormat::Color4Channel8BitNormalized_sRGB)
  {
    ezGALResourceFormat::Enum preferredFormat = ezGALResourceFormat::Invalid;
    if (const ezGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]))
    {
      auto rendertargetDesc = pTexture->GetDescription();

      preferredFormat = rendertargetDesc.m_Format;
    }

    switch (preferredFormat)
    {
      case ezGALResourceFormat::RGBAUByteNormalized:
      case ezGALResourceFormat::RGBAUByteNormalizedsRGB:
      default:
        if (format == ezSourceFormat::Color4Channel8BitNormalized_sRGB)
        {
          desc.m_Format = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
        }
        else
        {
          desc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
        }
        break;
      case ezGALResourceFormat::BGRAUByteNormalized:
      case ezGALResourceFormat::BGRAUByteNormalizedsRGB:
        if (format == ezSourceFormat::Color4Channel8BitNormalized_sRGB)
        {
          desc.m_Format = ezGALResourceFormat::BGRAUByteNormalizedsRGB;
        }
        else
        {
          desc.m_Format = ezGALResourceFormat::BGRAUByteNormalized;
        }
        break;
    }
  }
  else
  {
    switch (format)
    {
      case ezSourceFormat::Color4Channel16BitFloat:
        desc.m_Format = ezGALResourceFormat::RGBAHalf;
        break;
      case ezSourceFormat::Color4Channel32BitFloat:
        desc.m_Format = ezGALResourceFormat::RGBAFloat;
        break;
      case ezSourceFormat::Color3Channel11_11_10BitFloat:
      {
        const ezGALDeviceCapabilities& caps = ezGALDevice::GetDefaultDevice()->GetCapabilities();
        const bool bSupportsRG11B10Float = caps.m_FormatSupport[ezGALResourceFormat::RG11B10Float].AreAllSet(ezGALResourceFormatSupport::RenderTarget | ezGALResourceFormatSupport::Texture);
        desc.m_Format = bSupportsRG11B10Float ? ezGALResourceFormat::RG11B10Float : ezGALResourceFormat::RGBAHalf;
        break;
      }
      case ezSourceFormat::Depth16Bit:
        desc.m_Format = ezGALResourceFormat::D16;
        break;
      case ezSourceFormat::Depth24BitStencil8Bit:
        desc.m_Format = ezGALResourceFormat::D24S8;
        break;
      case ezSourceFormat::Depth32BitFloat:
        desc.m_Format = ezGALResourceFormat::DFloat;
        break;
      default:
        EZ_ASSERT_NOT_IMPLEMENTED
    }
  }

  desc.m_uiWidth = uiWidth;
  desc.m_uiHeight = uiHeight;
  desc.m_SampleCount = msaaMode;
  desc.m_bAllowRenderTargetView = true;
  desc.m_uiArraySize = view.GetCamera()->IsStereoscopic() ? 2 : 1;
  return desc;
}

bool ezSourcePass::GetRenderTargetDescriptions(
  const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  outputs[m_PinOutput.m_uiOutputIndex] = GetOutputDescription(view, m_Format, m_MsaaMode);
  return true;
}

void ezSourcePass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  if (!m_bClear)
    return;

  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Setup render target
  ezGALRenderingSetup renderingSetup;
  if (ezGALResourceFormat::IsDepthFormat(pOutput->m_Desc.m_Format))
  {
    renderingSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
    renderingSetup.SetClearDepth().SetClearStencil();
  }
  else
  {
    renderingSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
    renderingSetup.SetClearColor(0, m_ClearColor);
  }

  auto pCommandEncoder = ezRenderContext::BeginRenderingScope(renderViewContext, renderingSetup, GetName());
}

ezResult ezSourcePass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_Format;
  inout_stream << m_MsaaMode;
  inout_stream << m_ClearColor;
  inout_stream << m_bClear;
  return EZ_SUCCESS;
}

ezResult ezSourcePass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_Format;
  inout_stream >> m_MsaaMode;
  inout_stream >> m_ClearColor;
  inout_stream >> m_bClear;
  return EZ_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

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
    ezAbstractObjectNode::Property* formatProperty = pNode->FindProperty("Format");
    if (formatProperty == nullptr)
      return;

    auto formatName = formatProperty->m_Value.Get<ezString>();
    ezEnum<ezGALResourceFormat> oldFormat;
    ezReflectionUtils::StringToEnumeration<ezGALResourceFormat>(formatName.GetData(), oldFormat);

    ezEnum<ezSourceFormat> newFormat;

    switch (oldFormat)
    {
      case ezGALResourceFormat::RGBAHalf:
        newFormat = ezSourceFormat::Color4Channel16BitFloat;
        break;
      case ezGALResourceFormat::RGBAFloat:
        newFormat = ezSourceFormat::Color4Channel32BitFloat;
        break;
      case ezGALResourceFormat::RG11B10Float:
        newFormat = ezSourceFormat::Color3Channel11_11_10BitFloat;
        break;
      case ezGALResourceFormat::D16:
        newFormat = ezSourceFormat::Depth16Bit;
        break;
      case ezGALResourceFormat::D24S8:
        newFormat = ezSourceFormat::Depth24BitStencil8Bit;
        break;
      case ezGALResourceFormat::DFloat:
        newFormat = ezSourceFormat::Depth32BitFloat;
        break;
      case ezGALResourceFormat::RGBAUByteNormalized:
      case ezGALResourceFormat::BGRAUByteNormalized:
        newFormat = ezSourceFormat::Color4Channel8BitNormalized;
        break;
      case ezGALResourceFormat::RGBAUByteNormalizedsRGB:
      case ezGALResourceFormat::BGRAUByteNormalizedsRGB:
        newFormat = ezSourceFormat::Color4Channel8BitNormalized_sRGB;
        break;
      default:
        newFormat = ezSourceFormat::Default;
        break;
    }

    ezStringBuilder newFormatName;
    ezReflectionUtils::EnumerationToString(newFormat, newFormatName);
    formatProperty->m_Value = newFormatName.GetView();
  }
};

ezSourcePassPatch_2_3 g_ezSourcePassPatch_2_3;


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SourcePass);

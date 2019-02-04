#include <PCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTexture2DUsageEnum, 1)
  EZ_ENUM_CONSTANTS(ezTexture2DUsageEnum::Unknown, ezTexture2DUsageEnum::Diffuse)
  EZ_ENUM_CONSTANTS(ezTexture2DUsageEnum::NormalMap, ezTexture2DUsageEnum::NormalMapInverted)
  EZ_ENUM_CONSTANTS(ezTexture2DUsageEnum::EmissiveMask)
  EZ_ENUM_CONSTANTS(ezTexture2DUsageEnum::EmissiveColor, ezTexture2DUsageEnum::Height, ezTexture2DUsageEnum::Mask, ezTexture2DUsageEnum::LookupTable)
  EZ_ENUM_CONSTANTS(ezTexture2DUsageEnum::HDR)
  EZ_ENUM_CONSTANTS(ezTexture2DUsageEnum::Other_sRGB, ezTexture2DUsageEnum::Other_Linear)//, ezTexture2DUsageEnum::Other_sRGB_Auto, ezTexture2DUsageEnum::Other_Linear_Auto)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTexture2DChannelMappingEnum, 1)
  EZ_ENUM_CONSTANTS(ezTexture2DChannelMappingEnum::R1)
  EZ_ENUM_CONSTANTS(ezTexture2DChannelMappingEnum::RG1, ezTexture2DChannelMappingEnum::R1_G2)
  EZ_ENUM_CONSTANTS(ezTexture2DChannelMappingEnum::RGB1, ezTexture2DChannelMappingEnum::RGB1_ABLACK, ezTexture2DChannelMappingEnum::R1_G2_B3)
  EZ_ENUM_CONSTANTS(ezTexture2DChannelMappingEnum::RGBA1, ezTexture2DChannelMappingEnum::RGB1_A2, ezTexture2DChannelMappingEnum::R1_G2_B3_A4)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTexture2DAddressMode, 1)
  EZ_ENUM_CONSTANTS(ezTexture2DAddressMode::Wrap, ezTexture2DAddressMode::Mirror, ezTexture2DAddressMode::Clamp)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTexture2DResolution, 1)
  EZ_ENUM_CONSTANTS(ezTexture2DResolution::Fixed64x64, ezTexture2DResolution::Fixed128x128, ezTexture2DResolution::Fixed256x256, ezTexture2DResolution::Fixed512x512, ezTexture2DResolution::Fixed1024x1024, ezTexture2DResolution::Fixed2048x2048)
  EZ_ENUM_CONSTANTS(ezTexture2DResolution::CVarRtResolution1, ezTexture2DResolution::CVarRtResolution2)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezRenderTargetFormat, 1)
  EZ_ENUM_CONSTANTS(ezRenderTargetFormat::RGBA8sRgb, ezRenderTargetFormat::RGBA8, ezRenderTargetFormat::RGB10, ezRenderTargetFormat::RGBA16)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetProperties, 3, ezRTTIDefaultAllocator<ezTextureAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    /// \todo Accessor properties with enums don't link
    EZ_MEMBER_PROPERTY("IsRenderTarget", m_bIsRenderTarget)->AddAttributes(new ezHiddenAttribute),
    EZ_ENUM_MEMBER_PROPERTY("Usage", ezTexture2DUsageEnum, m_TextureUsage),

    EZ_ENUM_MEMBER_PROPERTY("Format", ezRenderTargetFormat, m_RtFormat),
    EZ_ENUM_MEMBER_PROPERTY("Resolution", ezTexture2DResolution, m_Resolution),
    EZ_MEMBER_PROPERTY("CVarResScale", m_fCVarResolutionScale)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 10.0f)),

    EZ_ENUM_MEMBER_PROPERTY("MipmapMode", ezTexConvMipmapMode, m_MipmapMode),
    EZ_ENUM_MEMBER_PROPERTY("CompressionMode", ezTexConvCompressionMode, m_CompressionMode),
    EZ_MEMBER_PROPERTY("PremultipliedAlpha", m_bPremultipliedAlpha),
    EZ_MEMBER_PROPERTY("FlipHorizontal", m_bFlipHorizontal),

    EZ_ENUM_MEMBER_PROPERTY("TextureFilter", ezTextureFilterSetting, m_TextureFilter),
    EZ_ENUM_MEMBER_PROPERTY("AddressModeU", ezTexture2DAddressMode, m_AddressModeU),
    EZ_ENUM_MEMBER_PROPERTY("AddressModeV", ezTexture2DAddressMode, m_AddressModeV),
    EZ_ENUM_MEMBER_PROPERTY("AddressModeW", ezTexture2DAddressMode, m_AddressModeW),

    EZ_ENUM_MEMBER_PROPERTY("ChannelMapping", ezTexture2DChannelMappingEnum, m_ChannelMapping),

    EZ_ACCESSOR_PROPERTY("Input1", GetInputFile0, SetInputFile0)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    EZ_ACCESSOR_PROPERTY("Input2", GetInputFile1, SetInputFile1)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    EZ_ACCESSOR_PROPERTY("Input3", GetInputFile2, SetInputFile2)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    EZ_ACCESSOR_PROPERTY("Input4", GetInputFile3, SetInputFile3)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),

  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezTextureAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezTextureAssetProperties>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool isRenderTarget = e.m_pObject->GetTypeAccessor().GetValue("IsRenderTarget").ConvertTo<bool>();

    if (isRenderTarget)
    {
      const ezInt32 resMode = e.m_pObject->GetTypeAccessor().GetValue("Resolution").ConvertTo<ezInt32>();
      const bool resIsCVar = resMode == ezTexture2DResolution::CVarRtResolution1 || resMode == ezTexture2DResolution::CVarRtResolution2;

      props["CVarResScale"].m_Visibility = resIsCVar ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
      props["Usage"].m_Visibility = ezPropertyUiState::Invisible;
      props["Mipmaps"].m_Visibility = ezPropertyUiState::Invisible;
      props["Compression"].m_Visibility = ezPropertyUiState::Invisible;
      props["PremultipliedAlpha"].m_Visibility = ezPropertyUiState::Invisible;
      props["FlipHorizontal"].m_Visibility = ezPropertyUiState::Invisible;
      props["ChannelMapping"].m_Visibility = ezPropertyUiState::Invisible;

      props["Input1"].m_Visibility = ezPropertyUiState::Invisible;
      props["Input2"].m_Visibility = ezPropertyUiState::Invisible;
      props["Input3"].m_Visibility = ezPropertyUiState::Invisible;
      props["Input4"].m_Visibility = ezPropertyUiState::Invisible;

      props["Format"].m_Visibility = ezPropertyUiState::Default;
      props["Resolution"].m_Visibility = ezPropertyUiState::Default;
    }
    else
    {
      props["CVarResScale"].m_Visibility = ezPropertyUiState::Invisible;
      props["Usage"].m_Visibility = ezPropertyUiState::Default;
      props["Mipmaps"].m_Visibility = ezPropertyUiState::Default;
      props["Compression"].m_Visibility = ezPropertyUiState::Default;
      props["PremultipliedAlpha"].m_Visibility = ezPropertyUiState::Default;
      props["FlipHorizontal"].m_Visibility = ezPropertyUiState::Default;
      props["ChannelMapping"].m_Visibility = ezPropertyUiState::Default;
      props["Format"].m_Visibility = ezPropertyUiState::Invisible;
      props["Resolution"].m_Visibility = ezPropertyUiState::Invisible;

      const ezInt64 mapping = e.m_pObject->GetTypeAccessor().GetValue("ChannelMapping").ConvertTo<ezInt64>();

      props["Usage"].m_Visibility = ezPropertyUiState::Default;
      props["Input1"].m_Visibility = ezPropertyUiState::Default;
      props["Input2"].m_Visibility = ezPropertyUiState::Invisible;
      props["Input3"].m_Visibility = ezPropertyUiState::Invisible;
      props["Input4"].m_Visibility = ezPropertyUiState::Invisible;

      {
        props["Input1"].m_sNewLabelText = "Input 1";
        props["Input2"].m_sNewLabelText = "Input 2";
        props["Input3"].m_sNewLabelText = "Input 3";
        props["Input4"].m_sNewLabelText = "Input 4";
      }

      switch (mapping)
      {
        case ezTexture2DChannelMappingEnum::R1_G2_B3_A4:
          props["Input4"].m_Visibility = ezPropertyUiState::Default;
          // fall through

        case ezTexture2DChannelMappingEnum::R1_G2_B3:
          props["Input3"].m_Visibility = ezPropertyUiState::Default;
          // fall through

        case ezTexture2DChannelMappingEnum::RGB1_A2:
        case ezTexture2DChannelMappingEnum::R1_G2:
          props["Input2"].m_Visibility = ezPropertyUiState::Default;
          break;
      }
    }
  }
}

ezString ezTextureAssetProperties::GetAbsoluteInputFilePath(ezInt32 iInput) const
{
  ezStringBuilder sPath = m_Input[iInput];
  sPath.MakeCleanPath();

  if (!sPath.IsAbsolutePath())
  {
    ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
  }

  return sPath;
}


ezInt32 ezTextureAssetProperties::GetNumInputFiles() const
{
  switch (m_ChannelMapping)
  {
    case ezTexture2DChannelMappingEnum::R1:
    case ezTexture2DChannelMappingEnum::RG1:
    case ezTexture2DChannelMappingEnum::RGB1:
    case ezTexture2DChannelMappingEnum::RGB1_ABLACK:
    case ezTexture2DChannelMappingEnum::RGBA1:
      return 1;

    case ezTexture2DChannelMappingEnum::R1_G2:
    case ezTexture2DChannelMappingEnum::RGB1_A2:
      return 2;

    case ezTexture2DChannelMappingEnum::R1_G2_B3:
      return 3;

    case ezTexture2DChannelMappingEnum::R1_G2_B3_A4:
      return 4;
  }

  EZ_REPORT_FAILURE("Invalid Code Path");
  return 1;
}


ezInt32 ezTextureAssetProperties::GetNumChannels() const
{
  if ((m_TextureUsage == ezTexture2DUsageEnum::NormalMap || m_TextureUsage == ezTexture2DUsageEnum::NormalMapInverted) && m_CompressionMode != ezTexConvCompressionMode::None)
  {
    return 2;
  }

  switch (m_ChannelMapping)
  {
    case ezTexture2DChannelMappingEnum::R1:
      return 1;

    case ezTexture2DChannelMappingEnum::RG1:
    case ezTexture2DChannelMappingEnum::R1_G2:
      return 2;

    case ezTexture2DChannelMappingEnum::RGB1:
    case ezTexture2DChannelMappingEnum::R1_G2_B3:
      return 3;

    case ezTexture2DChannelMappingEnum::RGB1_ABLACK:
    case ezTexture2DChannelMappingEnum::RGBA1:
    case ezTexture2DChannelMappingEnum::RGB1_A2:
    case ezTexture2DChannelMappingEnum::R1_G2_B3_A4:
      return 4;
  }

  EZ_REPORT_FAILURE("Invalid Code Path");
  return 4;
}

bool ezTextureAssetProperties::IsSRGB() const
{
  // these formats can never be sRGB
  if (m_ChannelMapping == ezTexture2DChannelMappingEnum::R1 || m_ChannelMapping == ezTexture2DChannelMappingEnum::R1_G2 ||
      m_ChannelMapping == ezTexture2DChannelMappingEnum::RG1)
    return false;

  if (m_TextureUsage == ezTexture2DUsageEnum::EmissiveMask || m_TextureUsage == ezTexture2DUsageEnum::Height ||
      m_TextureUsage == ezTexture2DUsageEnum::LookupTable || m_TextureUsage == ezTexture2DUsageEnum::Mask ||
      m_TextureUsage == ezTexture2DUsageEnum::NormalMap || m_TextureUsage == ezTexture2DUsageEnum::NormalMapInverted || m_TextureUsage == ezTexture2DUsageEnum::Other_Linear ||
      m_TextureUsage == ezTexture2DUsageEnum::HDR /* ||
      m_TextureUsage == ezTexture2DUsageEnum::Other_Linear_Auto*/)
    return false;

  return true;
}

bool ezTextureAssetProperties::IsHDR() const
{
  return m_TextureUsage == ezTexture2DUsageEnum::HDR;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezTextureAssetPropertiesPatch_2_3 : public ezGraphPatch
{
public:
  ezTextureAssetPropertiesPatch_2_3()
    : ezGraphPatch("ezTextureAssetProperties", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto* pMipmaps = pNode->FindProperty("Mipmaps");
    if (pMipmaps && pMipmaps->m_Value.IsA<bool>())
    {
      if (pMipmaps->m_Value.Get<bool>())
        pNode->AddProperty("MipmapMode", (ezInt32)ezTexConvMipmapMode::Linear);
      else
        pNode->AddProperty("MipmapMode", (ezInt32)ezTexConvMipmapMode::None);
    }

    auto* pCompression = pNode->FindProperty("Compression");
    if (pCompression && pCompression->m_Value.IsA<bool>())
    {
      if (pCompression->m_Value.Get<bool>())
        pNode->AddProperty("CompressionMode", (ezInt32)ezTexConvCompressionMode::Medium);
      else
        pNode->AddProperty("CompressionMode", (ezInt32)ezTexConvCompressionMode::None);
    }
  }
};

ezTextureAssetPropertiesPatch_2_3 g_ezTextureAssetPropertiesPatch_2_3;

#include <EditorPluginAssetsPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTexture2DChannelMappingEnum, 1)
  EZ_ENUM_CONSTANTS(ezTexture2DChannelMappingEnum::R1)
  EZ_ENUM_CONSTANTS(ezTexture2DChannelMappingEnum::RG1, ezTexture2DChannelMappingEnum::R1_G2)
  EZ_ENUM_CONSTANTS(ezTexture2DChannelMappingEnum::RGB1, ezTexture2DChannelMappingEnum::RGB1_ABLACK, ezTexture2DChannelMappingEnum::R1_G2_B3)
  EZ_ENUM_CONSTANTS(ezTexture2DChannelMappingEnum::RGBA1, ezTexture2DChannelMappingEnum::RGB1_A2, ezTexture2DChannelMappingEnum::R1_G2_B3_A4)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTexture2DResolution, 1)
  EZ_ENUM_CONSTANTS(ezTexture2DResolution::Fixed64x64, ezTexture2DResolution::Fixed128x128, ezTexture2DResolution::Fixed256x256, ezTexture2DResolution::Fixed512x512, ezTexture2DResolution::Fixed1024x1024, ezTexture2DResolution::Fixed2048x2048)
  EZ_ENUM_CONSTANTS(ezTexture2DResolution::CVarRtResolution1, ezTexture2DResolution::CVarRtResolution2)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezRenderTargetFormat, 1)
  EZ_ENUM_CONSTANTS(ezRenderTargetFormat::RGBA8sRgb, ezRenderTargetFormat::RGBA8, ezRenderTargetFormat::RGB10, ezRenderTargetFormat::RGBA16)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetProperties, 5, ezRTTIDefaultAllocator<ezTextureAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("IsRenderTarget", m_bIsRenderTarget)->AddAttributes(new ezHiddenAttribute),
    EZ_ENUM_MEMBER_PROPERTY("Usage", ezTexConvUsage, m_TextureUsage),

    EZ_ENUM_MEMBER_PROPERTY("Format", ezRenderTargetFormat, m_RtFormat),
    EZ_ENUM_MEMBER_PROPERTY("Resolution", ezTexture2DResolution, m_Resolution),
    EZ_MEMBER_PROPERTY("CVarResScale", m_fCVarResolutionScale)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 10.0f)),

    EZ_ENUM_MEMBER_PROPERTY("MipmapMode", ezTexConvMipmapMode, m_MipmapMode),
    EZ_MEMBER_PROPERTY("PreserveAlphaCoverage", m_bPreserveAlphaCoverage),
    EZ_MEMBER_PROPERTY("AlphaThreshold", m_fAlphaThreshold)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ENUM_MEMBER_PROPERTY("CompressionMode", ezTexConvCompressionMode, m_CompressionMode),
    EZ_MEMBER_PROPERTY("PremultipliedAlpha", m_bPremultipliedAlpha),
    EZ_MEMBER_PROPERTY("DilateColor", m_bDilateColor)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("FlipHorizontal", m_bFlipHorizontal),
    EZ_MEMBER_PROPERTY("HdrExposureBias", m_fHdrExposureBias)->AddAttributes(new ezClampValueAttribute(-20.0f, 20.0f)),

    EZ_ENUM_MEMBER_PROPERTY("TextureFilter", ezTextureFilterSetting, m_TextureFilter),
    EZ_ENUM_MEMBER_PROPERTY("AddressModeU", ezImageAddressMode, m_AddressModeU),
    EZ_ENUM_MEMBER_PROPERTY("AddressModeV", ezImageAddressMode, m_AddressModeV),
    EZ_ENUM_MEMBER_PROPERTY("AddressModeW", ezImageAddressMode, m_AddressModeW),

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

    props["AddressModeW"].m_Visibility = ezPropertyUiState::Invisible;

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
      props["PreserveAlphaCoverage"].m_Visibility = ezPropertyUiState::Invisible;
      props["AlphaThreshold"].m_Visibility = ezPropertyUiState::Invisible;
      props["PremultipliedAlpha"].m_Visibility = ezPropertyUiState::Invisible;
      props["HdrExposureBias"].m_Visibility = ezPropertyUiState::Invisible;

      props["Input1"].m_Visibility = ezPropertyUiState::Invisible;
      props["Input2"].m_Visibility = ezPropertyUiState::Invisible;
      props["Input3"].m_Visibility = ezPropertyUiState::Invisible;
      props["Input4"].m_Visibility = ezPropertyUiState::Invisible;

      props["Format"].m_Visibility = ezPropertyUiState::Default;
      props["Resolution"].m_Visibility = ezPropertyUiState::Default;
    }
    else
    {
      const bool hasMips = e.m_pObject->GetTypeAccessor().GetValue("MipmapMode").ConvertTo<ezInt32>() != ezTexConvMipmapMode::None;
      const bool isHDR = e.m_pObject->GetTypeAccessor().GetValue("Usage").ConvertTo<ezInt32>() == ezTexConvUsage::Hdr;

      props["CVarResScale"].m_Visibility = ezPropertyUiState::Invisible;
      props["Usage"].m_Visibility = ezPropertyUiState::Default;
      props["Mipmaps"].m_Visibility = ezPropertyUiState::Default;
      props["Compression"].m_Visibility = ezPropertyUiState::Default;
      props["PremultipliedAlpha"].m_Visibility = ezPropertyUiState::Disabled;
      props["FlipHorizontal"].m_Visibility = ezPropertyUiState::Default;
      props["ChannelMapping"].m_Visibility = ezPropertyUiState::Default;
      props["Format"].m_Visibility = ezPropertyUiState::Invisible;
      props["Resolution"].m_Visibility = ezPropertyUiState::Invisible;
      props["PreserveAlphaCoverage"].m_Visibility = ezPropertyUiState::Disabled;
      props["AlphaThreshold"].m_Visibility = ezPropertyUiState::Disabled;
      props["PremultipliedAlpha"].m_Visibility = ezPropertyUiState::Disabled;
      props["HdrExposureBias"].m_Visibility = ezPropertyUiState::Disabled;

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

      if (mapping == ezTexture2DChannelMappingEnum::RGBA1 || mapping == ezTexture2DChannelMappingEnum::R1_G2_B3_A4 ||
          mapping == ezTexture2DChannelMappingEnum::RGB1_A2 || mapping == ezTexture2DChannelMappingEnum::R1_G2_B3_A4)
      {
        props["PremultipliedAlpha"].m_Visibility = ezPropertyUiState::Default;

        if (hasMips)
        {
          props["PreserveAlphaCoverage"].m_Visibility = ezPropertyUiState::Default;
          props["AlphaThreshold"].m_Visibility = ezPropertyUiState::Default;
        }
      }

      if (isHDR)
      {
        props["HdrExposureBias"].m_Visibility = ezPropertyUiState::Default;
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
        pNode->AddProperty("MipmapMode", (ezInt32)ezTexConvMipmapMode::Kaiser);
      else
        pNode->AddProperty("MipmapMode", (ezInt32)ezTexConvMipmapMode::None);
    }

    auto* pCompression = pNode->FindProperty("Compression");
    if (pCompression && pCompression->m_Value.IsA<bool>())
    {
      if (pCompression->m_Value.Get<bool>())
        pNode->AddProperty("CompressionMode", (ezInt32)ezTexConvCompressionMode::High);
      else
        pNode->AddProperty("CompressionMode", (ezInt32)ezTexConvCompressionMode::None);
    }
  }
};

ezTextureAssetPropertiesPatch_2_3 g_ezTextureAssetPropertiesPatch_2_3;

//////////////////////////////////////////////////////////////////////////

class ezTextureAssetPropertiesPatch_3_4 : public ezGraphPatch
{
public:
  ezTextureAssetPropertiesPatch_3_4()
    : ezGraphPatch("ezTextureAssetProperties", 4)
  {
  }

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    const char* szAddressModes[] = {"AddressModeU", "AddressModeV", "AddressModeW"};

    for (ezUInt32 i = 0; i < 3; ++i)
    {
      auto* pAddress = pNode->FindProperty(szAddressModes[i]);
      if (pAddress && pAddress->m_Value.IsA<ezString>())
      {
        if (pAddress->m_Value.Get<ezString>() == "ezTexture2DAddressMode::Wrap")
        {
          pNode->ChangeProperty(szAddressModes[i], (ezInt32)ezImageAddressMode::Repeat);
        }
        else if (pAddress->m_Value.Get<ezString>() == "ezTexture2DAddressMode::Clamp")
        {
          pNode->ChangeProperty(szAddressModes[i], (ezInt32)ezImageAddressMode::Clamp);
        }
        else if (pAddress->m_Value.Get<ezString>() == "ezTexture2DAddressMode::Mirror")
        {
          pNode->ChangeProperty(szAddressModes[i], (ezInt32)ezImageAddressMode::Mirror);
        }
      }
    }
  }
};

ezTextureAssetPropertiesPatch_3_4 g_ezTextureAssetPropertiesPatch_3_4;

//////////////////////////////////////////////////////////////////////////

class ezTextureAssetPropertiesPatch_4_5 : public ezGraphPatch
{
public:
  ezTextureAssetPropertiesPatch_4_5()
    : ezGraphPatch("ezTextureAssetProperties", 5)
  {
  }

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto* pUsage = pNode->FindProperty("Usage");
    if (pUsage && pUsage->m_Value.IsA<ezString>())
    {
      if (pUsage->m_Value.Get<ezString>() == "ezTexture2DUsageEnum::Unknown")
      {
        pNode->ChangeProperty("Usage", (ezInt32)ezTexConvUsage::Auto);
      }
      else if (pUsage->m_Value.Get<ezString>() == "ezTexture2DUsageEnum::Other_sRGB" ||
               pUsage->m_Value.Get<ezString>() == "ezTexture2DUsageEnum::Diffuse" ||
               pUsage->m_Value.Get<ezString>() == "ezTexture2DUsageEnum::EmissiveColor")
      {
        pNode->ChangeProperty("Usage", (ezInt32)ezTexConvUsage::Color);
      }
      else if (pUsage->m_Value.Get<ezString>() == "ezTexture2DUsageEnum::Height" ||
               pUsage->m_Value.Get<ezString>() == "ezTexture2DUsageEnum::Mask" ||
               pUsage->m_Value.Get<ezString>() == "ezTexture2DUsageEnum::LookupTable" ||
               pUsage->m_Value.Get<ezString>() == "ezTexture2DUsageEnum::Other_Linear" ||
               pUsage->m_Value.Get<ezString>() == "ezTexture2DUsageEnum::EmissiveMask")
      {
        pNode->ChangeProperty("Usage", (ezInt32)ezTexConvUsage::Linear);
      }
      else if (pUsage->m_Value.Get<ezString>() == "ezTexture2DUsageEnum::NormalMap")
      {
        pNode->ChangeProperty("Usage", (ezInt32)ezTexConvUsage::NormalMap);
      }
      else if (pUsage->m_Value.Get<ezString>() == "ezTexture2DUsageEnum::HDR")
      {
        pNode->ChangeProperty("Usage", (ezInt32)ezTexConvUsage::Hdr);
      }
    }
  }
};

ezTextureAssetPropertiesPatch_4_5 g_ezTextureAssetPropertiesPatch_4_5;

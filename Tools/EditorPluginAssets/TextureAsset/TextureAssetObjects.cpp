#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>


EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTexture2DUsageEnum, 1)
EZ_ENUM_CONSTANTS(ezTexture2DUsageEnum::Unknown, ezTexture2DUsageEnum::Diffuse, ezTexture2DUsageEnum::NormalMap, ezTexture2DUsageEnum::EmissiveMask)
EZ_ENUM_CONSTANTS(ezTexture2DUsageEnum::EmissiveColor, ezTexture2DUsageEnum::Height, ezTexture2DUsageEnum::Mask, ezTexture2DUsageEnum::LookupTable)
EZ_ENUM_CONSTANTS(ezTexture2DUsageEnum::HDR)
EZ_ENUM_CONSTANTS(ezTexture2DUsageEnum::Other_sRGB, ezTexture2DUsageEnum::Other_Linear)//, ezTexture2DUsageEnum::Other_sRGB_Auto, ezTexture2DUsageEnum::Other_Linear_Auto)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTexture2DChannelMappingEnum, 1)
EZ_ENUM_CONSTANTS(ezTexture2DChannelMappingEnum::R1_2D)
EZ_ENUM_CONSTANTS(ezTexture2DChannelMappingEnum::RG1_2D, ezTexture2DChannelMappingEnum::R1_G2_2D)
EZ_ENUM_CONSTANTS(ezTexture2DChannelMappingEnum::RGB1_2D, ezTexture2DChannelMappingEnum::RGB1_ABLACK_2D, ezTexture2DChannelMappingEnum::R1_G2_B3_2D)
EZ_ENUM_CONSTANTS(ezTexture2DChannelMappingEnum::RGBA1_2D, ezTexture2DChannelMappingEnum::RGB1_A2_2D, ezTexture2DChannelMappingEnum::R1_G2_B3_A4_2D)
EZ_ENUM_CONSTANTS(ezTexture2DChannelMappingEnum::RGB1_CUBE, ezTexture2DChannelMappingEnum::RGBA1_CUBE, ezTexture2DChannelMappingEnum::RGB1TO6_CUBE, ezTexture2DChannelMappingEnum::RGBA1TO6_CUBE)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTexture2DAddressMode, 1)
EZ_ENUM_CONSTANTS(ezTexture2DAddressMode::Wrap, ezTexture2DAddressMode::Mirror, ezTexture2DAddressMode::Clamp)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetProperties, 2, ezRTTIDefaultAllocator<ezTextureAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    /// \todo Accessor properties with enums don't link
    EZ_ENUM_MEMBER_PROPERTY("Usage", ezTexture2DUsageEnum, m_TextureUsage),

    EZ_MEMBER_PROPERTY("Mipmaps", m_bMipmaps)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Compression", m_bCompression)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("PremultipliedAlpha", m_bPremultipliedAlpha),

    EZ_ENUM_MEMBER_PROPERTY("TextureFilter", ezTextureFilterSetting, m_TextureFilter),
    EZ_ENUM_MEMBER_PROPERTY("AddressModeU", ezTexture2DAddressMode, m_AddressModeU),
    EZ_ENUM_MEMBER_PROPERTY("AddressModeV", ezTexture2DAddressMode, m_AddressModeV),
    EZ_ENUM_MEMBER_PROPERTY("AddressModeW", ezTexture2DAddressMode, m_AddressModeW),

    EZ_ENUM_MEMBER_PROPERTY("ChannelMapping", ezTexture2DChannelMappingEnum, m_ChannelMapping),

    EZ_ACCESSOR_PROPERTY("Input1", GetInputFile0, SetInputFile0)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    EZ_ACCESSOR_PROPERTY("Input2", GetInputFile1, SetInputFile1)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    EZ_ACCESSOR_PROPERTY("Input3", GetInputFile2, SetInputFile2)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    EZ_ACCESSOR_PROPERTY("Input4", GetInputFile3, SetInputFile3)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    EZ_ACCESSOR_PROPERTY("Input5", GetInputFile4, SetInputFile4)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    EZ_ACCESSOR_PROPERTY("Input6", GetInputFile5, SetInputFile5)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),

  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezTextureAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezRTTI::FindTypeByName("ezTextureAssetProperties"))
  {
    const ezInt64 mapping = e.m_pObject->GetTypeAccessor().GetValue("ChannelMapping").ConvertTo<ezInt64>();

    auto& props = *e.m_pPropertyStates;

    props["Usage"].m_Visibility = ezPropertyUiState::Default;
    props["Input1"].m_Visibility = ezPropertyUiState::Default;
    props["Input2"].m_Visibility = ezPropertyUiState::Invisible;
    props["Input3"].m_Visibility = ezPropertyUiState::Invisible;
    props["Input4"].m_Visibility = ezPropertyUiState::Invisible;
    props["Input5"].m_Visibility = ezPropertyUiState::Invisible;
    props["Input6"].m_Visibility = ezPropertyUiState::Invisible;

    if (mapping == ezTexture2DChannelMappingEnum::RGB1TO6_CUBE || mapping == ezTexture2DChannelMappingEnum::RGBA1TO6_CUBE)
    {
      props["Input1"].m_sNewLabelText = "Right (+X)";
      props["Input2"].m_sNewLabelText = "Left (-X)";
      props["Input3"].m_sNewLabelText = "Top (+Y)";
      props["Input4"].m_sNewLabelText = "Bottom (-Y)";
      props["Input5"].m_sNewLabelText = "Front (+Z)";
      props["Input6"].m_sNewLabelText = "Back (-Z)";
    }
    else
    {
      props["Input1"].m_sNewLabelText = "Input 1";
      props["Input2"].m_sNewLabelText = "Input 2";
      props["Input3"].m_sNewLabelText = "Input 3";
      props["Input4"].m_sNewLabelText = "Input 4";
      props["Input5"].m_sNewLabelText = "Input 5";
      props["Input6"].m_sNewLabelText = "Input 6";
    }

    switch (mapping)
    {
    case ezTexture2DChannelMappingEnum::R1_G2_2D:
      props["Input2"].m_Visibility = ezPropertyUiState::Default;
      // fall through

    case ezTexture2DChannelMappingEnum::RG1_2D:
    case ezTexture2DChannelMappingEnum::R1_2D:
      props["Usage"].m_Visibility = ezPropertyUiState::Disabled;
      break;


    case ezTexture2DChannelMappingEnum::RGB1TO6_CUBE:
    case ezTexture2DChannelMappingEnum::RGBA1TO6_CUBE:
      props["Input6"].m_Visibility = ezPropertyUiState::Default;
      props["Input5"].m_Visibility = ezPropertyUiState::Default;
      // fall through

    case ezTexture2DChannelMappingEnum::R1_G2_B3_A4_2D:
      props["Input4"].m_Visibility = ezPropertyUiState::Default;
      // fall through

    case ezTexture2DChannelMappingEnum::R1_G2_B3_2D:
      props["Input3"].m_Visibility = ezPropertyUiState::Default;
      // fall through

    case ezTexture2DChannelMappingEnum::RGB1_A2_2D:
      props["Input2"].m_Visibility = ezPropertyUiState::Default;
      // fall through
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
  case ezTexture2DChannelMappingEnum::R1_2D:
  case ezTexture2DChannelMappingEnum::RG1_2D:
  case ezTexture2DChannelMappingEnum::RGB1_2D:
  case ezTexture2DChannelMappingEnum::RGB1_ABLACK_2D:
  case ezTexture2DChannelMappingEnum::RGBA1_2D:
  case ezTexture2DChannelMappingEnum::RGB1_CUBE:
  case ezTexture2DChannelMappingEnum::RGBA1_CUBE:
    return 1;

  case ezTexture2DChannelMappingEnum::R1_G2_2D:
  case ezTexture2DChannelMappingEnum::RGB1_A2_2D:
    return 2;

  case ezTexture2DChannelMappingEnum::R1_G2_B3_2D:
    return 3;

  case ezTexture2DChannelMappingEnum::R1_G2_B3_A4_2D:
    return 4;

  case ezTexture2DChannelMappingEnum::RGB1TO6_CUBE:
  case ezTexture2DChannelMappingEnum::RGBA1TO6_CUBE:
    return 6;
  }

  EZ_REPORT_FAILURE("Invalid Code Path");
  return 1;
}


ezInt32 ezTextureAssetProperties::GetNumChannels() const
{
  switch (m_ChannelMapping)
  {
  case ezTexture2DChannelMappingEnum::R1_2D:
    return 1;

  case ezTexture2DChannelMappingEnum::RG1_2D:
  case ezTexture2DChannelMappingEnum::R1_G2_2D:
    return 2;

  case ezTexture2DChannelMappingEnum::RGB1_2D:
  case ezTexture2DChannelMappingEnum::R1_G2_B3_2D:
  case ezTexture2DChannelMappingEnum::RGB1_CUBE:
  case ezTexture2DChannelMappingEnum::RGB1TO6_CUBE:
    return 3;

  case ezTexture2DChannelMappingEnum::RGB1_ABLACK_2D:
  case ezTexture2DChannelMappingEnum::RGBA1_2D:
  case ezTexture2DChannelMappingEnum::RGB1_A2_2D:
  case ezTexture2DChannelMappingEnum::R1_G2_B3_A4_2D:
  case ezTexture2DChannelMappingEnum::RGBA1_CUBE:
  case ezTexture2DChannelMappingEnum::RGBA1TO6_CUBE:
    return 4;
  }

  EZ_REPORT_FAILURE("Invalid Code Path");
  return 4;
}

bool ezTextureAssetProperties::IsSRGB() const
{
  // these formats can never be sRGB
  if (m_ChannelMapping == ezTexture2DChannelMappingEnum::R1_2D ||
      m_ChannelMapping == ezTexture2DChannelMappingEnum::R1_G2_2D ||
      m_ChannelMapping == ezTexture2DChannelMappingEnum::RG1_2D)
    return false;

  if (m_TextureUsage == ezTexture2DUsageEnum::EmissiveMask ||
      m_TextureUsage == ezTexture2DUsageEnum::Height ||
      m_TextureUsage == ezTexture2DUsageEnum::LookupTable ||
      m_TextureUsage == ezTexture2DUsageEnum::Mask ||
      m_TextureUsage == ezTexture2DUsageEnum::NormalMap ||
      m_TextureUsage == ezTexture2DUsageEnum::Other_Linear ||
      m_TextureUsage == ezTexture2DUsageEnum::HDR/* ||
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

class ezTextureAssetPropertiesPatch_1_2 : public ezGraphPatch
{
public:
  ezTextureAssetPropertiesPatch_1_2()
    : ezGraphPatch(ezGetStaticRTTI<ezTextureAssetProperties>(), 2) {}

  virtual void Patch(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Premultiplied Alpha", "PremultipliedAlpha");
    pNode->RenameProperty("Texture Filter", "TextureFilter");
    pNode->RenameProperty("Address Mode U", "AddressModeU");
    pNode->RenameProperty("Address Mode V", "AddressModeV");
    pNode->RenameProperty("Address Mode W", "AddressModeW");
    pNode->RenameProperty("Channel Mapping", "ChannelMapping");
    pNode->RenameProperty("Input 1", "Input1");
    pNode->RenameProperty("Input 2", "Input2");
    pNode->RenameProperty("Input 3", "Input3");
    pNode->RenameProperty("Input 4", "Input4");
    pNode->RenameProperty("Input 5", "Input5");
    pNode->RenameProperty("Input 6", "Input6");
  }
};

ezTextureAssetPropertiesPatch_1_2 g_ezTextureAssetPropertiesPatch_1_2;



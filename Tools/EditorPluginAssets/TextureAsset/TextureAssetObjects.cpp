#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>


EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureUsageEnum, 1)
EZ_ENUM_CONSTANTS(ezTextureUsageEnum::Unknown, ezTextureUsageEnum::Diffuse, ezTextureUsageEnum::NormalMap, ezTextureUsageEnum::EmissiveMask)
EZ_ENUM_CONSTANTS(ezTextureUsageEnum::EmissiveColor, ezTextureUsageEnum::Height, ezTextureUsageEnum::Mask, ezTextureUsageEnum::LookupTable)
EZ_ENUM_CONSTANTS(ezTextureUsageEnum::HDR)
EZ_ENUM_CONSTANTS(ezTextureUsageEnum::Other_sRGB, ezTextureUsageEnum::Other_Linear)//, ezTextureUsageEnum::Other_sRGB_Auto, ezTextureUsageEnum::Other_Linear_Auto)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezChannelMappingEnum, 1)
EZ_ENUM_CONSTANTS(ezChannelMappingEnum::R1_2D)
EZ_ENUM_CONSTANTS(ezChannelMappingEnum::RG1_2D, ezChannelMappingEnum::R1_G2_2D)
EZ_ENUM_CONSTANTS(ezChannelMappingEnum::RGB1_2D, ezChannelMappingEnum::RGB1_ABLACK_2D, ezChannelMappingEnum::R1_G2_B3_2D)
EZ_ENUM_CONSTANTS(ezChannelMappingEnum::RGBA1_2D, ezChannelMappingEnum::RGB1_A2_2D, ezChannelMappingEnum::R1_G2_B3_A4_2D)
EZ_ENUM_CONSTANTS(ezChannelMappingEnum::RGB1_CUBE, ezChannelMappingEnum::RGBA1_CUBE, ezChannelMappingEnum::RGB1TO6_CUBE, ezChannelMappingEnum::RGBA1TO6_CUBE)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureAddressMode, 1)
EZ_ENUM_CONSTANTS(ezTextureAddressMode::Wrap, ezTextureAddressMode::Mirror, ezTextureAddressMode::Clamp)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetProperties, 2, ezRTTIDefaultAllocator<ezTextureAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    /// \todo Accessor properties with enums don't link
    EZ_ENUM_MEMBER_PROPERTY("Usage", ezTextureUsageEnum, m_TextureUsage),

    EZ_MEMBER_PROPERTY("Mipmaps", m_bMipmaps)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Compression", m_bCompression)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("PremultipliedAlpha", m_bPremultipliedAlpha),

    EZ_ENUM_MEMBER_PROPERTY("TextureFilter", ezTextureFilterSetting, m_TextureFilter),
    EZ_ENUM_MEMBER_PROPERTY("AddressModeU", ezTextureAddressMode, m_AddressModeU),
    EZ_ENUM_MEMBER_PROPERTY("AddressModeV", ezTextureAddressMode, m_AddressModeV),
    EZ_ENUM_MEMBER_PROPERTY("AddressModeW", ezTextureAddressMode, m_AddressModeW),

    EZ_ENUM_MEMBER_PROPERTY("ChannelMapping", ezChannelMappingEnum, m_ChannelMapping),

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

    if (mapping == ezChannelMappingEnum::RGB1TO6_CUBE || mapping == ezChannelMappingEnum::RGBA1TO6_CUBE)
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
    case ezChannelMappingEnum::R1_G2_2D:
      props["Input2"].m_Visibility = ezPropertyUiState::Default;
      // fall through

    case ezChannelMappingEnum::RG1_2D:
    case ezChannelMappingEnum::R1_2D:
      props["Usage"].m_Visibility = ezPropertyUiState::Disabled;
      break;


    case ezChannelMappingEnum::RGB1TO6_CUBE:
    case ezChannelMappingEnum::RGBA1TO6_CUBE:
      props["Input6"].m_Visibility = ezPropertyUiState::Default;
      props["Input5"].m_Visibility = ezPropertyUiState::Default;
      // fall through

    case ezChannelMappingEnum::R1_G2_B3_A4_2D:
      props["Input4"].m_Visibility = ezPropertyUiState::Default;
      // fall through

    case ezChannelMappingEnum::R1_G2_B3_2D:
      props["Input3"].m_Visibility = ezPropertyUiState::Default;
      // fall through

    case ezChannelMappingEnum::RGB1_A2_2D:
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
  case ezChannelMappingEnum::R1_2D:
  case ezChannelMappingEnum::RG1_2D:
  case ezChannelMappingEnum::RGB1_2D:
  case ezChannelMappingEnum::RGB1_ABLACK_2D:
  case ezChannelMappingEnum::RGBA1_2D:
  case ezChannelMappingEnum::RGB1_CUBE:
  case ezChannelMappingEnum::RGBA1_CUBE:
    return 1;

  case ezChannelMappingEnum::R1_G2_2D:
  case ezChannelMappingEnum::RGB1_A2_2D:
    return 2;

  case ezChannelMappingEnum::R1_G2_B3_2D:
    return 3;

  case ezChannelMappingEnum::R1_G2_B3_A4_2D:
    return 4;

  case ezChannelMappingEnum::RGB1TO6_CUBE:
  case ezChannelMappingEnum::RGBA1TO6_CUBE:
    return 6;
  }

  EZ_REPORT_FAILURE("Invalid Code Path");
  return 1;
}


ezInt32 ezTextureAssetProperties::GetNumChannels() const
{
  switch (m_ChannelMapping)
  {
  case ezChannelMappingEnum::R1_2D:
    return 1;

  case ezChannelMappingEnum::RG1_2D:
  case ezChannelMappingEnum::R1_G2_2D:
    return 2;

  case ezChannelMappingEnum::RGB1_2D:
  case ezChannelMappingEnum::R1_G2_B3_2D:
  case ezChannelMappingEnum::RGB1_CUBE:
  case ezChannelMappingEnum::RGB1TO6_CUBE:
    return 3;

  case ezChannelMappingEnum::RGB1_ABLACK_2D:
  case ezChannelMappingEnum::RGBA1_2D:
  case ezChannelMappingEnum::RGB1_A2_2D:
  case ezChannelMappingEnum::R1_G2_B3_A4_2D:
  case ezChannelMappingEnum::RGBA1_CUBE:
  case ezChannelMappingEnum::RGBA1TO6_CUBE:
    return 4;
  }

  EZ_REPORT_FAILURE("Invalid Code Path");
  return 4;
}

bool ezTextureAssetProperties::IsSRGB() const
{
  // these formats can never be sRGB
  if (m_ChannelMapping == ezChannelMappingEnum::R1_2D ||
      m_ChannelMapping == ezChannelMappingEnum::R1_G2_2D ||
      m_ChannelMapping == ezChannelMappingEnum::RG1_2D)
    return false;

  if (m_TextureUsage == ezTextureUsageEnum::EmissiveMask ||
      m_TextureUsage == ezTextureUsageEnum::Height ||
      m_TextureUsage == ezTextureUsageEnum::LookupTable ||
      m_TextureUsage == ezTextureUsageEnum::Mask ||
      m_TextureUsage == ezTextureUsageEnum::NormalMap ||
      m_TextureUsage == ezTextureUsageEnum::Other_Linear ||
      m_TextureUsage == ezTextureUsageEnum::HDR/* ||
      m_TextureUsage == ezTextureUsageEnum::Other_Linear_Auto*/)
    return false;


  return true;
}

bool ezTextureAssetProperties::IsHDR() const
{
  return m_TextureUsage == ezTextureUsageEnum::HDR;
}

bool ezTextureAssetProperties::IsTexture2D() const
{
  return (m_ChannelMapping == ezChannelMappingEnum::R1_2D ||
          m_ChannelMapping == ezChannelMappingEnum::R1_G2_2D ||
          m_ChannelMapping == ezChannelMappingEnum::R1_G2_B3_2D ||
          m_ChannelMapping == ezChannelMappingEnum::R1_G2_B3_A4_2D ||
          m_ChannelMapping == ezChannelMappingEnum::RG1_2D ||
          m_ChannelMapping == ezChannelMappingEnum::RGB1_2D ||
          m_ChannelMapping == ezChannelMappingEnum::RGB1_ABLACK_2D ||
          m_ChannelMapping == ezChannelMappingEnum::RGB1_A2_2D ||
          m_ChannelMapping == ezChannelMappingEnum::RGBA1_2D);
}

bool ezTextureAssetProperties::IsTextureCube() const
{
  return (m_ChannelMapping == ezChannelMappingEnum::RGB1_CUBE ||
          m_ChannelMapping == ezChannelMappingEnum::RGBA1_CUBE ||
          m_ChannelMapping == ezChannelMappingEnum::RGB1TO6_CUBE ||
          m_ChannelMapping == ezChannelMappingEnum::RGBA1TO6_CUBE);
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



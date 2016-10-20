#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>


EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureUsageEnum, 1)
EZ_ENUM_CONSTANTS(ezTextureUsageEnum::Unknown, ezTextureUsageEnum::Diffuse, ezTextureUsageEnum::NormalMap, ezTextureUsageEnum::EmissiveMask)
EZ_ENUM_CONSTANTS(ezTextureUsageEnum::EmissiveColor, ezTextureUsageEnum::Height, ezTextureUsageEnum::Mask, ezTextureUsageEnum::LookupTable)
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetProperties, 1, ezRTTIDefaultAllocator<ezTextureAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    /// \todo Accessor properties with enums don't link
    EZ_ENUM_MEMBER_PROPERTY("Usage", ezTextureUsageEnum, m_TextureUsage),

    EZ_MEMBER_PROPERTY("Mipmaps", m_bMipmaps)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Compression", m_bCompression)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Premultiplied Alpha", m_bPremultipliedAlpha),

    EZ_ENUM_MEMBER_PROPERTY("Address Mode U", ezTextureAddressMode, m_AddressModeU),
    EZ_ENUM_MEMBER_PROPERTY("Address Mode V", ezTextureAddressMode, m_AddressModeV),
    EZ_ENUM_MEMBER_PROPERTY("Address Mode W", ezTextureAddressMode, m_AddressModeW),

    EZ_ENUM_MEMBER_PROPERTY("Channel Mapping", ezChannelMappingEnum, m_ChannelMapping),

    EZ_ACCESSOR_PROPERTY("Input 1", GetInputFile0, SetInputFile0)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png")),
    EZ_ACCESSOR_PROPERTY("Input 2", GetInputFile1, SetInputFile1)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png")),
    EZ_ACCESSOR_PROPERTY("Input 3", GetInputFile2, SetInputFile2)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png")),
    EZ_ACCESSOR_PROPERTY("Input 4", GetInputFile3, SetInputFile3)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png")),
    EZ_ACCESSOR_PROPERTY("Input 5", GetInputFile4, SetInputFile4)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png")),
    EZ_ACCESSOR_PROPERTY("Input 6", GetInputFile5, SetInputFile5)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png")),

  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezTextureAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezRTTI::FindTypeByName("ezTextureAssetProperties"))
  {
    const ezInt64 mapping = e.m_pObject->GetTypeAccessor().GetValue("Channel Mapping").ConvertTo<ezInt64>();

    auto& props = *e.m_pPropertyStates;

    props["Usage"].m_Visibility = ezPropertyUiState::Default;
    props["Input 1"].m_Visibility = ezPropertyUiState::Default;
    props["Input 2"].m_Visibility = ezPropertyUiState::Invisible;
    props["Input 3"].m_Visibility = ezPropertyUiState::Invisible;
    props["Input 4"].m_Visibility = ezPropertyUiState::Invisible;
    props["Input 5"].m_Visibility = ezPropertyUiState::Invisible;
    props["Input 6"].m_Visibility = ezPropertyUiState::Invisible;

    if (mapping == ezChannelMappingEnum::RGB1TO6_CUBE || mapping == ezChannelMappingEnum::RGBA1TO6_CUBE)
    {
      props["Input 1"].m_sNewLabelText = "Right (+X)";
      props["Input 2"].m_sNewLabelText = "Left (-X)";
      props["Input 3"].m_sNewLabelText = "Top (+Y)";
      props["Input 4"].m_sNewLabelText = "Bottom (-Y)";
      props["Input 5"].m_sNewLabelText = "Front (+Z)";
      props["Input 6"].m_sNewLabelText = "Back (-Z)";
    }
    else
    {
      props["Input 1"].m_sNewLabelText = "Input 1";
      props["Input 2"].m_sNewLabelText = "Input 2";
      props["Input 3"].m_sNewLabelText = "Input 3";
      props["Input 4"].m_sNewLabelText = "Input 4";
      props["Input 5"].m_sNewLabelText = "Input 5";
      props["Input 6"].m_sNewLabelText = "Input 6";
    }

    switch (mapping)
    {
    case ezChannelMappingEnum::R1_G2_2D:
      props["Input 2"].m_Visibility = ezPropertyUiState::Default;
      // fall through

    case ezChannelMappingEnum::RG1_2D:
    case ezChannelMappingEnum::R1_2D:
      props["Usage"].m_Visibility = ezPropertyUiState::Disabled;
      break;


    case ezChannelMappingEnum::RGB1TO6_CUBE:
    case ezChannelMappingEnum::RGBA1TO6_CUBE:
      props["Input 6"].m_Visibility = ezPropertyUiState::Default;
      props["Input 5"].m_Visibility = ezPropertyUiState::Default;
      // fall through

    case ezChannelMappingEnum::R1_G2_B3_A4_2D:
      props["Input 4"].m_Visibility = ezPropertyUiState::Default;
      // fall through

    case ezChannelMappingEnum::R1_G2_B3_2D:
      props["Input 3"].m_Visibility = ezPropertyUiState::Default;
      // fall through

    case ezChannelMappingEnum::RGB1_A2_2D:
      props["Input 2"].m_Visibility = ezPropertyUiState::Default;
      // fall through
    }
  }
}

ezString ezTextureAssetProperties::GetAbsoluteInputFilePath(ezInt32 iInput) const
{
  ezStringBuilder sTemp = m_Input[iInput];
  sTemp.MakeCleanPath();

  ezString sPath = sTemp;

  if (!sTemp.IsAbsolutePath())
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
      m_TextureUsage == ezTextureUsageEnum::Other_Linear/* ||
      m_TextureUsage == ezTextureUsageEnum::Other_Linear_Auto*/)
    return false;


  return true;
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


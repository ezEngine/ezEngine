#include <PCH.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>


EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureCubeUsageEnum, 1)
EZ_ENUM_CONSTANTS(ezTextureCubeUsageEnum::Unknown, ezTextureCubeUsageEnum::Diffuse, ezTextureCubeUsageEnum::NormalMap, ezTextureCubeUsageEnum::EmissiveMask)
EZ_ENUM_CONSTANTS(ezTextureCubeUsageEnum::EmissiveColor, ezTextureCubeUsageEnum::Height, ezTextureCubeUsageEnum::Mask, ezTextureCubeUsageEnum::LookupTable)
EZ_ENUM_CONSTANTS(ezTextureCubeUsageEnum::HDR)
EZ_ENUM_CONSTANTS(ezTextureCubeUsageEnum::Other_sRGB, ezTextureCubeUsageEnum::Other_Linear)//, ezTextureCubeUsageEnum::Other_sRGB_Auto, ezTextureCubeUsageEnum::Other_Linear_Auto)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureCubeChannelMappingEnum, 1)
EZ_ENUM_CONSTANTS(ezTextureCubeChannelMappingEnum::R1_2D)
EZ_ENUM_CONSTANTS(ezTextureCubeChannelMappingEnum::RG1_2D, ezTextureCubeChannelMappingEnum::R1_G2_2D)
EZ_ENUM_CONSTANTS(ezTextureCubeChannelMappingEnum::RGB1_2D, ezTextureCubeChannelMappingEnum::RGB1_ABLACK_2D, ezTextureCubeChannelMappingEnum::R1_G2_B3_2D)
EZ_ENUM_CONSTANTS(ezTextureCubeChannelMappingEnum::RGBA1_2D, ezTextureCubeChannelMappingEnum::RGB1_A2_2D, ezTextureCubeChannelMappingEnum::R1_G2_B3_A4_2D)
EZ_ENUM_CONSTANTS(ezTextureCubeChannelMappingEnum::RGB1_CUBE, ezTextureCubeChannelMappingEnum::RGBA1_CUBE, ezTextureCubeChannelMappingEnum::RGB1TO6_CUBE, ezTextureCubeChannelMappingEnum::RGBA1TO6_CUBE)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureCubeAssetProperties, 2, ezRTTIDefaultAllocator<ezTextureCubeAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    /// \todo Accessor properties with enums don't link
    EZ_ENUM_MEMBER_PROPERTY("Usage", ezTextureCubeUsageEnum, m_TextureUsage),

    EZ_MEMBER_PROPERTY("Mipmaps", m_bMipmaps)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Compression", m_bCompression)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("PremultipliedAlpha", m_bPremultipliedAlpha),

    EZ_ENUM_MEMBER_PROPERTY("TextureFilter", ezTextureFilterSetting, m_TextureFilter),

    EZ_ENUM_MEMBER_PROPERTY("ChannelMapping", ezTextureCubeChannelMappingEnum, m_ChannelMapping),

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

void ezTextureCubeAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezRTTI::FindTypeByName("ezTextureCubeAssetProperties"))
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

    if (mapping == ezTextureCubeChannelMappingEnum::RGB1TO6_CUBE || mapping == ezTextureCubeChannelMappingEnum::RGBA1TO6_CUBE)
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
    case ezTextureCubeChannelMappingEnum::R1_G2_2D:
      props["Input2"].m_Visibility = ezPropertyUiState::Default;
      // fall through

    case ezTextureCubeChannelMappingEnum::RG1_2D:
    case ezTextureCubeChannelMappingEnum::R1_2D:
      props["Usage"].m_Visibility = ezPropertyUiState::Disabled;
      break;


    case ezTextureCubeChannelMappingEnum::RGB1TO6_CUBE:
    case ezTextureCubeChannelMappingEnum::RGBA1TO6_CUBE:
      props["Input6"].m_Visibility = ezPropertyUiState::Default;
      props["Input5"].m_Visibility = ezPropertyUiState::Default;
      // fall through

    case ezTextureCubeChannelMappingEnum::R1_G2_B3_A4_2D:
      props["Input4"].m_Visibility = ezPropertyUiState::Default;
      // fall through

    case ezTextureCubeChannelMappingEnum::R1_G2_B3_2D:
      props["Input3"].m_Visibility = ezPropertyUiState::Default;
      // fall through

    case ezTextureCubeChannelMappingEnum::RGB1_A2_2D:
      props["Input2"].m_Visibility = ezPropertyUiState::Default;
      // fall through
    }
  }
}

ezString ezTextureCubeAssetProperties::GetAbsoluteInputFilePath(ezInt32 iInput) const
{
  ezStringBuilder sPath = m_Input[iInput];
  sPath.MakeCleanPath();

  if (!sPath.IsAbsolutePath())
  {
    ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
  }

  return sPath;
}


ezInt32 ezTextureCubeAssetProperties::GetNumInputFiles() const
{
  switch (m_ChannelMapping)
  {
  case ezTextureCubeChannelMappingEnum::R1_2D:
  case ezTextureCubeChannelMappingEnum::RG1_2D:
  case ezTextureCubeChannelMappingEnum::RGB1_2D:
  case ezTextureCubeChannelMappingEnum::RGB1_ABLACK_2D:
  case ezTextureCubeChannelMappingEnum::RGBA1_2D:
  case ezTextureCubeChannelMappingEnum::RGB1_CUBE:
  case ezTextureCubeChannelMappingEnum::RGBA1_CUBE:
    return 1;

  case ezTextureCubeChannelMappingEnum::R1_G2_2D:
  case ezTextureCubeChannelMappingEnum::RGB1_A2_2D:
    return 2;

  case ezTextureCubeChannelMappingEnum::R1_G2_B3_2D:
    return 3;

  case ezTextureCubeChannelMappingEnum::R1_G2_B3_A4_2D:
    return 4;

  case ezTextureCubeChannelMappingEnum::RGB1TO6_CUBE:
  case ezTextureCubeChannelMappingEnum::RGBA1TO6_CUBE:
    return 6;
  }

  EZ_REPORT_FAILURE("Invalid Code Path");
  return 1;
}


ezInt32 ezTextureCubeAssetProperties::GetNumChannels() const
{
  switch (m_ChannelMapping)
  {
  case ezTextureCubeChannelMappingEnum::R1_2D:
    return 1;

  case ezTextureCubeChannelMappingEnum::RG1_2D:
  case ezTextureCubeChannelMappingEnum::R1_G2_2D:
    return 2;

  case ezTextureCubeChannelMappingEnum::RGB1_2D:
  case ezTextureCubeChannelMappingEnum::R1_G2_B3_2D:
  case ezTextureCubeChannelMappingEnum::RGB1_CUBE:
  case ezTextureCubeChannelMappingEnum::RGB1TO6_CUBE:
    return 3;

  case ezTextureCubeChannelMappingEnum::RGB1_ABLACK_2D:
  case ezTextureCubeChannelMappingEnum::RGBA1_2D:
  case ezTextureCubeChannelMappingEnum::RGB1_A2_2D:
  case ezTextureCubeChannelMappingEnum::R1_G2_B3_A4_2D:
  case ezTextureCubeChannelMappingEnum::RGBA1_CUBE:
  case ezTextureCubeChannelMappingEnum::RGBA1TO6_CUBE:
    return 4;
  }

  EZ_REPORT_FAILURE("Invalid Code Path");
  return 4;
}

bool ezTextureCubeAssetProperties::IsSRGB() const
{
  // these formats can never be sRGB
  if (m_ChannelMapping == ezTextureCubeChannelMappingEnum::R1_2D ||
      m_ChannelMapping == ezTextureCubeChannelMappingEnum::R1_G2_2D ||
      m_ChannelMapping == ezTextureCubeChannelMappingEnum::RG1_2D)
    return false;

  if (m_TextureUsage == ezTextureCubeUsageEnum::EmissiveMask ||
      m_TextureUsage == ezTextureCubeUsageEnum::Height ||
      m_TextureUsage == ezTextureCubeUsageEnum::LookupTable ||
      m_TextureUsage == ezTextureCubeUsageEnum::Mask ||
      m_TextureUsage == ezTextureCubeUsageEnum::NormalMap ||
      m_TextureUsage == ezTextureCubeUsageEnum::Other_Linear ||
      m_TextureUsage == ezTextureCubeUsageEnum::HDR/* ||
      m_TextureUsage == ezTextureCubeUsageEnum::Other_Linear_Auto*/)
    return false;


  return true;
}

bool ezTextureCubeAssetProperties::IsHDR() const
{
  return m_TextureUsage == ezTextureCubeUsageEnum::HDR;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezTextureCubeAssetPropertiesPatch_1_2 : public ezGraphPatch
{
public:
  ezTextureCubeAssetPropertiesPatch_1_2()
    : ezGraphPatch(ezGetStaticRTTI<ezTextureCubeAssetProperties>(), 2) {}

  virtual void Patch(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Premultiplied Alpha", "PremultipliedAlpha");
    pNode->RenameProperty("Texture Filter", "TextureFilter");
    pNode->RenameProperty("Channel Mapping", "ChannelMapping");
    pNode->RenameProperty("Input 1", "Input1");
    pNode->RenameProperty("Input 2", "Input2");
    pNode->RenameProperty("Input 3", "Input3");
    pNode->RenameProperty("Input 4", "Input4");
    pNode->RenameProperty("Input 5", "Input5");
    pNode->RenameProperty("Input 6", "Input6");
  }
};

ezTextureCubeAssetPropertiesPatch_1_2 g_ezTextureCubeAssetPropertiesPatch_1_2;



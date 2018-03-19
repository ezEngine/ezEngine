#include <PCH.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>


EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureCubeUsageEnum, 1)
EZ_ENUM_CONSTANTS(ezTextureCubeUsageEnum::Unknown, ezTextureCubeUsageEnum::Skybox, ezTextureCubeUsageEnum::SkyboxHDR, ezTextureCubeUsageEnum::LookupTable)
EZ_ENUM_CONSTANTS(ezTextureCubeUsageEnum::Other_sRGB, ezTextureCubeUsageEnum::Other_Linear)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureCubeChannelMappingEnum, 1)
EZ_ENUM_CONSTANTS(ezTextureCubeChannelMappingEnum::RGB1, ezTextureCubeChannelMappingEnum::RGBA1, ezTextureCubeChannelMappingEnum::RGB1TO6, ezTextureCubeChannelMappingEnum::RGBA1TO6)
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
  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezTextureCubeAssetProperties>())
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

    if (mapping == ezTextureCubeChannelMappingEnum::RGB1TO6 || mapping == ezTextureCubeChannelMappingEnum::RGBA1TO6)
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
    case ezTextureCubeChannelMappingEnum::RGB1TO6:
    case ezTextureCubeChannelMappingEnum::RGBA1TO6:
      props["Input6"].m_Visibility = ezPropertyUiState::Default;
      props["Input5"].m_Visibility = ezPropertyUiState::Default;
      props["Input4"].m_Visibility = ezPropertyUiState::Default;
      props["Input3"].m_Visibility = ezPropertyUiState::Default;
      props["Input2"].m_Visibility = ezPropertyUiState::Default;
      break;
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
  case ezTextureCubeChannelMappingEnum::RGB1:
  case ezTextureCubeChannelMappingEnum::RGBA1:
    return 1;

  case ezTextureCubeChannelMappingEnum::RGB1TO6:
  case ezTextureCubeChannelMappingEnum::RGBA1TO6:
    return 6;
  }

  EZ_REPORT_FAILURE("Invalid Code Path");
  return 1;
}


ezInt32 ezTextureCubeAssetProperties::GetNumChannels() const
{
  switch (m_ChannelMapping)
  {
  case ezTextureCubeChannelMappingEnum::RGB1:
  case ezTextureCubeChannelMappingEnum::RGB1TO6:
    return 3;

  case ezTextureCubeChannelMappingEnum::RGBA1:
  case ezTextureCubeChannelMappingEnum::RGBA1TO6:
    return 4;
  }

  EZ_REPORT_FAILURE("Invalid Code Path");
  return 4;
}

bool ezTextureCubeAssetProperties::IsSRGB() const
{
  if (m_TextureUsage == ezTextureCubeUsageEnum::SkyboxHDR ||
      m_TextureUsage == ezTextureCubeUsageEnum::LookupTable ||
      m_TextureUsage == ezTextureCubeUsageEnum::Other_Linear)
    return false;


  return true;
}

bool ezTextureCubeAssetProperties::IsHDR() const
{
  return m_TextureUsage == ezTextureCubeUsageEnum::SkyboxHDR;
}


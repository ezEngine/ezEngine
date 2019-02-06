#include <PCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureCubeChannelMappingEnum, 1)
  EZ_ENUM_CONSTANTS(ezTextureCubeChannelMappingEnum::RGB1, ezTextureCubeChannelMappingEnum::RGBA1, ezTextureCubeChannelMappingEnum::RGB1TO6, ezTextureCubeChannelMappingEnum::RGBA1TO6)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureCubeAssetProperties, 3, ezRTTIDefaultAllocator<ezTextureCubeAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Usage", ezTexConvUsage, m_TextureUsage),

    EZ_ENUM_MEMBER_PROPERTY("MipmapMode", ezTexConvMipmapMode, m_MipmapMode),
    EZ_ENUM_MEMBER_PROPERTY("CompressionMode", ezTexConvCompressionMode, m_CompressionMode),

    EZ_ENUM_MEMBER_PROPERTY("TextureFilter", ezTextureFilterSetting, m_TextureFilter),

    EZ_ENUM_MEMBER_PROPERTY("ChannelMapping", ezTextureCubeChannelMappingEnum, m_ChannelMapping),

    EZ_ACCESSOR_PROPERTY("Input1", GetInputFile0, SetInputFile0)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    EZ_ACCESSOR_PROPERTY("Input2", GetInputFile1, SetInputFile1)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    EZ_ACCESSOR_PROPERTY("Input3", GetInputFile2, SetInputFile2)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    EZ_ACCESSOR_PROPERTY("Input4", GetInputFile3, SetInputFile3)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    EZ_ACCESSOR_PROPERTY("Input5", GetInputFile4, SetInputFile4)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),
    EZ_ACCESSOR_PROPERTY("Input6", GetInputFile5, SetInputFile5)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr")),

  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

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

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezTextureCubeAssetProperties_2_3 : public ezGraphPatch
{
public:
  ezTextureCubeAssetProperties_2_3()
    : ezGraphPatch("ezTextureCubeAssetProperties", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto* pUsage = pNode->FindProperty("Usage");
    if (pUsage && pUsage->m_Value.IsA<ezString>())
    {
      if (pUsage->m_Value.Get<ezString>() == "ezTextureCubeUsageEnum::Unknown")
      {
        pNode->ChangeProperty("Usage", (ezInt32)ezTexConvUsage::Auto);
      }
      else if (pUsage->m_Value.Get<ezString>() == "ezTextureCubeUsageEnum::Other_sRGB" ||
               pUsage->m_Value.Get<ezString>() == "ezTextureCubeUsageEnum::Skybox")
      {
        pNode->ChangeProperty("Usage", (ezInt32)ezTexConvUsage::Color);
      }
      else if (pUsage->m_Value.Get<ezString>() == "ezTextureCubeUsageEnum::Other_Linear" ||
               pUsage->m_Value.Get<ezString>() == "ezTextureCubeUsageEnum::LookupTable")
      {
        pNode->ChangeProperty("Usage", (ezInt32)ezTexConvUsage::Linear);
      }
      else if (pUsage->m_Value.Get<ezString>() == "ezTextureCubeUsageEnum::SkyboxHDR")
      {
        pNode->ChangeProperty("Usage", (ezInt32)ezTexConvUsage::Hdr);
      }
    }

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
        pNode->AddProperty("CompressionMode", (ezInt32)ezTexConvCompressionMode::Medium);
      else
        pNode->AddProperty("CompressionMode", (ezInt32)ezTexConvCompressionMode::None);
    }
  }
};

ezTextureCubeAssetProperties_2_3 g_ezTextureCubeAssetProperties_2_3;

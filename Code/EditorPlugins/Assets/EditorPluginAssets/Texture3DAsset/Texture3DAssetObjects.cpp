#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/Texture3DAsset/Texture3DAssetObjects.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTexture3DAssetProperties, 1, ezRTTIDefaultAllocator<ezTexture3DAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Input", GetInputFile, SetInputFile)->AddAttributes(new ezFileBrowserAttribute("Select Volume Texture", "*.dds")),

    // "Auto" usage detection only works for 2D textures, so default to something that works for volume data.
    EZ_ENUM_MEMBER_PROPERTY("Usage", ezTexConvUsage, m_TextureUsage)->AddAttributes(new ezDefaultValueAttribute((ezInt32)ezTexConvUsage::Linear)),
    EZ_ENUM_MEMBER_PROPERTY("MipmapMode", ezTexConvMipmapMode, m_MipmapMode),
    EZ_ENUM_MEMBER_PROPERTY("CompressionMode", ezTexConvCompressionMode, m_CompressionMode),
    EZ_MEMBER_PROPERTY("HdrExposureBias", m_fHdrExposureBias)->AddAttributes(new ezClampValueAttribute(-20.0f, 20.0f)),

    EZ_ENUM_MEMBER_PROPERTY("TextureFilter", ezTextureFilterSetting, m_TextureFilter),
    EZ_ENUM_MEMBER_PROPERTY("AddressModeU", ezImageAddressMode, m_AddressModeU),
    EZ_ENUM_MEMBER_PROPERTY("AddressModeV", ezImageAddressMode, m_AddressModeV),
    EZ_ENUM_MEMBER_PROPERTY("AddressModeW", ezImageAddressMode, m_AddressModeW),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezTexture3DAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezTexture3DAssetProperties>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool isHDR = e.m_pObject->GetTypeAccessor().GetValue("Usage").ConvertTo<ezInt32>() == ezTexConvUsage::Hdr;

    props["HdrExposureBias"].m_Visibility = isHDR ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
  }
}

ezString ezTexture3DAssetProperties::GetAbsoluteInputFilePath() const
{
  ezStringBuilder sPath = m_sInput;
  sPath.MakeCleanPath();

  if (!sPath.IsAbsolutePath())
  {
    ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
  }

  return sPath;
}

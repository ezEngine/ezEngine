#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>


EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureUsageEnum, 1)
  EZ_ENUM_CONSTANTS(ezTextureUsageEnum::Unknown, ezTextureUsageEnum::Diffuse, ezTextureUsageEnum::NormalMap, ezTextureUsageEnum::EmissiveMask)
  EZ_ENUM_CONSTANTS(ezTextureUsageEnum::EmissiveColor, ezTextureUsageEnum::Height, ezTextureUsageEnum::Mask, ezTextureUsageEnum::LookupTable, ezTextureUsageEnum::Skybox)
  EZ_ENUM_CONSTANTS(ezTextureUsageEnum::Other_sRGB, ezTextureUsageEnum::Other_Linear, ezTextureUsageEnum::Other_sRGB_Auto, ezTextureUsageEnum::Other_Linear_Auto)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureTypeEnum, 1)
  EZ_ENUM_CONSTANTS(ezTextureTypeEnum::Unknown, ezTextureTypeEnum::Texture2D, ezTextureTypeEnum::Texture3D, ezTextureTypeEnum::TextureCube)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetProperties, 1, ezRTTIDefaultAllocator<ezTextureAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Texture File", GetInputFile, SetInputFile)->AddAttributes(new ezFileBrowserAttribute("Select Texture", "*.dds;*.tga")),
    /// \todo Accessor properties with enums don't link
    EZ_ENUM_MEMBER_PROPERTY("Usage", ezTextureUsageEnum, m_TextureUsage),
    EZ_ENUM_MEMBER_PROPERTY_READ_ONLY("Type", ezTextureTypeEnum, m_TextureType),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Format", GetFormatString),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Width", GetWidth),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Height", GetHeight),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Depth", GetDepth),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezTextureAssetProperties::ezTextureAssetProperties()
{
}

ezString ezTextureAssetProperties::GetFormatString() const
{
  return ezImageFormat::GetName(m_Image.GetImageFormat());
}

void ezTextureAssetProperties::SetInputFile(const char* szFile)
{
  ezStringBuilder sTemp = szFile;
  sTemp.MakeCleanPath();

  ezString sPath = sTemp;

  if (!sTemp.IsAbsolutePath())
  {
    ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
  }

  m_TextureType = ezTextureTypeEnum::Unknown;

  m_Input = sTemp;
  if (m_Image.LoadFrom(sPath).Succeeded())
  {
    if (m_Image.GetNumFaces() == 6)
      m_TextureType = ezTextureTypeEnum::TextureCube;
    else if (m_Image.GetDepth() > 1)
      m_TextureType = ezTextureTypeEnum::Texture3D;
    else
      m_TextureType = ezTextureTypeEnum::Texture2D;
  }
}

bool ezTextureAssetProperties::IsSRGB() const
{
  if (m_TextureUsage == ezTextureUsageEnum::EmissiveMask ||
      m_TextureUsage == ezTextureUsageEnum::Height ||
      m_TextureUsage == ezTextureUsageEnum::LookupTable ||
      m_TextureUsage == ezTextureUsageEnum::Mask ||
      m_TextureUsage == ezTextureUsageEnum::NormalMap ||
      m_TextureUsage == ezTextureUsageEnum::Other_Linear ||
      m_TextureUsage == ezTextureUsageEnum::Other_Linear_Auto)
    return false;


  return true;
}


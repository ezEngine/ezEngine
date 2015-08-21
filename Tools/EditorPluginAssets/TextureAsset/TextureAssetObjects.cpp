#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>


EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureUsageEnum, 1)
EZ_ENUM_CONSTANTS(ezTextureUsageEnum::Unknown, ezTextureUsageEnum::Diffuse, ezTextureUsageEnum::NormalMap, ezTextureUsageEnum::Height, ezTextureUsageEnum::Mask, ezTextureUsageEnum::LookupTable, ezTextureUsageEnum::Skybox)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureTypeEnum, 1)
  EZ_ENUM_CONSTANTS(ezTextureTypeEnum::Unknown, ezTextureTypeEnum::Texture2D, ezTextureTypeEnum::Texture3D, ezTextureTypeEnum::TextureCube)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSRGBModeEnum, 1)
  EZ_ENUM_CONSTANTS(ezSRGBModeEnum::Unknown, ezSRGBModeEnum::sRGB, ezSRGBModeEnum::Linear, ezSRGBModeEnum::sRGB_Auto, ezSRGBModeEnum::Linear_Auto)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetProperties, ezReflectedClass, 1, ezRTTIDefaultAllocator<ezTextureAssetProperties>);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Texture File", GetInputFile, SetInputFile),
    //EZ_ACCESSOR_PROPERTY("Usage", GetTextureUsage, SetTextureUsage),
    /// \todo Accessor properties with enums don't link
    EZ_ENUM_MEMBER_PROPERTY("Usage", ezTextureUsageEnum, m_TextureUsage),
    EZ_ENUM_MEMBER_PROPERTY("sRGB-Mode", ezSRGBModeEnum, m_sRGBMode),
    EZ_ENUM_MEMBER_PROPERTY_READ_ONLY("Type", ezTextureTypeEnum, m_TextureType),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Format", GetFormatString),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Width", GetWidth),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Height", GetHeight),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Depth", GetDepth),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTextureAssetProperties::ezTextureAssetProperties()
{
  m_sRGBMode = ezSRGBModeEnum::Unknown;
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
    ezEditorApp::GetInstance()->MakeDataDirectoryRelativePathAbsolute(sPath);
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

void ezTextureAssetProperties::SetTextureUsage(ezEnum<ezTextureUsageEnum> usage)
{
  /// \todo Currently does not work, because enum accessors don't compile

  m_TextureUsage = usage;

  if (m_sRGBMode == ezSRGBModeEnum::Unknown || m_sRGBMode == ezSRGBModeEnum::Linear_Auto || m_sRGBMode == ezSRGBModeEnum::sRGB_Auto)
  {
    switch (m_TextureUsage)
    {
    case ezTextureUsageEnum::Diffuse:
    case ezTextureUsageEnum::Skybox:
      m_sRGBMode = ezSRGBModeEnum::sRGB_Auto;
      break;
    case ezTextureUsageEnum::NormalMap:
    case ezTextureUsageEnum::Height:
    case ezTextureUsageEnum::Mask:
    case ezTextureUsageEnum::LookupTable:
      m_sRGBMode = ezSRGBModeEnum::Linear_Auto;
      break;

    default:
      break;
    }
  }
}

bool ezTextureAssetProperties::IsSRGB() const
{
  return (m_sRGBMode == ezSRGBModeEnum::Unknown || m_sRGBMode == ezSRGBModeEnum::sRGB || m_sRGBMode == ezSRGBModeEnum::sRGB_Auto);
}


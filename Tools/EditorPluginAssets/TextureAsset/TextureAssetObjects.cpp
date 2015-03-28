#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>


EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureUsageEnum, 1)
  EZ_ENUM_CONSTANTS(ezTextureUsageEnum::DiffuseMap, ezTextureUsageEnum::NormalMap) 
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetProperties, ezReflectedClass, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Texture File", GetInputFile, SetInputFile),
    EZ_MEMBER_PROPERTY("sRGB", m_bIsSRGB),
    EZ_ENUM_MEMBER_PROPERTY("Usage", ezTextureUsageEnum, m_TextureUsage),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Format", GetFormatString),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Width", GetWidth),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Height", GetHeight),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Depth", GetDepth),
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Is Cubemap", IsCubemap),


  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTextureAssetProperties::ezTextureAssetProperties()
{
  m_bIsSRGB = true;
}

ezString ezTextureAssetProperties::GetFormatString() const
{
  return ezImageFormat::GetName(m_Image.GetImageFormat());
}

void ezTextureAssetProperties::SetInputFile(const char* szFile)
{
  m_Input = szFile;
  if (m_Image.LoadFrom(szFile).Succeeded())
  {
  }
}




ezTextureAssetObject::ezTextureAssetObject()
{
}

ezTextureAssetObject::~ezTextureAssetObject()
{
}


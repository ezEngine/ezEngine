#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <CoreUtils/Image/Image.h>

struct ezTextureUsageEnum
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    Unknown,
    Diffuse,
    NormalMap,
    Height,
    Mask,
    LookupTable,
    Skybox,
    Default = Unknown,
  };
};

struct ezTextureTypeEnum
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    Unknown,
    Texture2D,
    Texture3D,
    TextureCube,
    Default = Unknown,
  };
};

struct ezSRGBModeEnum
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    Unknown,
    sRGB,
    Linear,
    sRGB_Auto,
    Linear_Auto,
    Default = Unknown,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTextureUsageEnum);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTextureTypeEnum);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezSRGBModeEnum);

class ezTextureAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureAssetProperties);

public:
  ezTextureAssetProperties();

  void SetInputFile(const char* szFile);
  const char* GetInputFile() const { return m_Input; }

  ezUInt32 GetWidth() const { return m_Image.GetWidth(); }
  ezUInt32 GetHeight() const { return m_Image.GetHeight(); }
  ezUInt32 GetDepth() const { return m_Image.GetDepth(); }
  const ezImage& GetImage() const { return m_Image; }
  ezString GetFormatString() const;
  ezTextureTypeEnum::Enum GetTextureType() const { return m_TextureType; }
  bool IsSRGB() const;

  void SetTextureUsage(ezEnum<ezTextureUsageEnum> usage);
  ezEnum<ezTextureUsageEnum> GetTextureUsage() const { return m_TextureUsage; }

private:
  ezString m_Input;
  ezEnum <ezSRGBModeEnum> m_sRGBMode;
  ezEnum<ezTextureUsageEnum> m_TextureUsage;
  ezEnum<ezTextureTypeEnum> m_TextureType;

  ezImage m_Image;
};

#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <CoreUtils/Image/Image.h>

struct ezTextureUsageEnum
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    DiffuseMap,
    NormalMap,
    Default = DiffuseMap,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezTextureUsageEnum);



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
  bool IsSRGB() const { return m_bIsSRGB; }
  bool IsCubemap() const { return m_Image.GetNumFaces() == 6; }
  const ezImage& GetImage() const { return m_Image; }
  ezString GetFormatString() const;

private:
  ezString m_Input;
  bool m_bIsSRGB;
  ezEnum<ezTextureUsageEnum> m_TextureUsage;

  ezImage m_Image;
};

class ezTextureAssetObject : public ezDocumentObjectDirectMember<ezReflectedClass, ezTextureAssetProperties>
{
public:
  ezTextureAssetObject();
  ~ezTextureAssetObject();
};



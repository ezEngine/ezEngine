#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Reflection/Reflection.h>

/// \brief Base class for asset type specific transform settings
///
///
class EZ_EDITORFRAMEWORK_DLL ezAssetTypePlatformConfig : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetTypePlatformConfig, ezReflectedClass);

public:
  ezAssetTypePlatformConfig();
  ~ezAssetTypePlatformConfig();

  virtual const char* GetDisplayName() const = 0;
};

class EZ_EDITORFRAMEWORK_DLL ezTextureAssetTypePlatformConfig : public ezAssetTypePlatformConfig
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureAssetTypePlatformConfig, ezAssetTypePlatformConfig);

public:
  virtual const char* GetDisplayName() const override;

  ezUInt16 m_uiMaxResolution = 1024 * 16;
};

struct ezAssetTargetPlatform
{
  enum Enum
  {
    PC,
    Android,

    Default = PC
  };

  typedef ezUInt8 StorageType;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezAssetTargetPlatform);

class EZ_EDITORFRAMEWORK_DLL ezAssetPlatformConfig : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetPlatformConfig, ezReflectedClass);

public:
  ezAssetPlatformConfig();
  ~ezAssetPlatformConfig();

  const char* GetConfigName() const { return m_sName; }

  void InitializeToDefault();
  void Clear();

  template <typename TYPE>
  const TYPE* GetTypeConfig() const
  {
    return static_cast<const TYPE*>(GetTypeConfig(ezGetStaticRTTI<TYPE>()));
  }

  template <typename TYPE>
  TYPE* GetTypeConfig()
  {
    return static_cast<TYPE*>(GetTypeConfig(ezGetStaticRTTI<TYPE>()));
  }

  const ezAssetTypePlatformConfig* GetTypeConfig(const ezRTTI* pRtti) const;
  ezAssetTypePlatformConfig* GetTypeConfig(const ezRTTI* pRtti);

  ezString m_sName;
  ezEnum<ezAssetTargetPlatform> m_TargetPlatform;
  ezDynamicArray<ezAssetTypePlatformConfig*> m_TypeConfigs;
};

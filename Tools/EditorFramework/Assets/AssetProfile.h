#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Reflection/Reflection.h>

/// \brief Base class for asset type specific transform settings
///
///
class EZ_EDITORFRAMEWORK_DLL ezAssetTypeProfileConfig : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetTypeProfileConfig, ezReflectedClass);

public:
  ezAssetTypeProfileConfig();
  ~ezAssetTypeProfileConfig();

  virtual const char* GetDisplayName() const = 0;
};

struct ezAssetProfileTargetPlatform
{
  enum Enum
  {
    PC,
    Android,

    Default = PC
  };

  typedef ezUInt8 StorageType;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezAssetProfileTargetPlatform);

class EZ_EDITORFRAMEWORK_DLL ezAssetProfile : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetProfile, ezReflectedClass);

public:
  ezAssetProfile();
  ~ezAssetProfile();

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

  const ezAssetTypeProfileConfig* GetTypeConfig(const ezRTTI* pRtti) const;
  ezAssetTypeProfileConfig* GetTypeConfig(const ezRTTI* pRtti);

  ezString m_sName;
  ezEnum<ezAssetProfileTargetPlatform> m_TargetPlatform;
  ezDynamicArray<ezAssetTypeProfileConfig*> m_TypeConfigs;
};

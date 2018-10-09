#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/Basics.h>

class ezChunkStreamWriter;
class ezChunkStreamReader;

struct ezProfileTargetPlatform
{
  enum Enum
  {
    PC,
    UWP,
    Android,

    Default = PC
  };

  typedef ezUInt8 StorageType;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezProfileTargetPlatform);

//////////////////////////////////////////////////////////////////////////

/// \brief Base class for configuration objects that store e.g. asset transform settings or runtime configuration information
class EZ_GAMEENGINE_DLL ezProfileConfigData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProfileConfigData, ezReflectedClass);

public:
  ezProfileConfigData();
  ~ezProfileConfigData();

  virtual void SaveRuntimeData(ezChunkStreamWriter& stream) const;
  virtual void LoadRuntimeData(ezChunkStreamReader& stream);
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezPlatformProfile : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPlatformProfile, ezReflectedClass);

public:
  ezPlatformProfile();
  ~ezPlatformProfile();

  const char* GetConfigName() const { return m_sName; }

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

  const ezProfileConfigData* GetTypeConfig(const ezRTTI* pRtti) const;
  ezProfileConfigData* GetTypeConfig(const ezRTTI* pRtti);

  ezResult SaveForRuntime(const char* szFile) const;
  ezResult LoadForRuntime(const char* szFile);

  ezString m_sName;
  ezEnum<ezProfileTargetPlatform> m_TargetPlatform;
  ezDynamicArray<ezProfileConfigData*> m_Configs;
};

//////////////////////////////////////////////////////////////////////////

class ezRenderPipelineProfileConfig : public ezProfileConfigData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelineProfileConfig, ezProfileConfigData);

public:
  virtual void SaveRuntimeData(ezChunkStreamWriter& stream) const override;
  virtual void LoadRuntimeData(ezChunkStreamReader& stream) override;

  ezString m_sMainRenderPipeline;
  ezString m_sEditorRenderPipeline;
  ezString m_sDebugRenderPipeline;
  ezString m_sShadowMapRenderPipeline;

  ezMap<ezString, ezString> m_CameraPipelines;
};

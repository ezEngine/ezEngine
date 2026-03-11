#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class ezChunkStreamWriter;
class ezChunkStreamReader;

//////////////////////////////////////////////////////////////////////////

/// \brief Base class for configuration objects that store e.g. asset transform settings or runtime configuration information
class EZ_CORE_DLL ezProfileConfigData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProfileConfigData, ezReflectedClass);

public:
  ezProfileConfigData();
  ~ezProfileConfigData();

  virtual void SaveRuntimeData(ezChunkStreamWriter& inout_stream) const;
  virtual void LoadRuntimeData(ezChunkStreamReader& inout_stream);
};

//////////////////////////////////////////////////////////////////////////

/// \brief Stores platform-specific configuration data for asset processing and runtime settings.
///
/// A platform profile contains multiple configuration objects (ezProfileConfigData) that store
/// settings for different aspects like asset transforms, rendering options, etc. Each profile
/// targets a specific platform and maintains a modification counter for change tracking.
class EZ_CORE_DLL ezPlatformProfile final : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPlatformProfile, ezReflectedClass);

public:
  ezPlatformProfile();
  ~ezPlatformProfile();

  void SetConfigName(ezStringView sName) { m_sName = sName; }
  ezStringView GetConfigName() const { return m_sName; }

  void SetTargetPlatform(ezStringView sPlatform) { m_sTargetPlatform = sPlatform; }
  ezStringView GetTargetPlatform() const { return m_sTargetPlatform; }

  void Clear();
  void AddMissingConfigs();

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

  ezResult SaveForRuntime(ezStringView sFile) const;
  ezResult LoadForRuntime(ezStringView sFile);

  /// \brief Returns a number indicating when the profile counter changed last. By storing and comparing this value, other code can update their state if necessary.
  ezUInt32 GetLastModificationCounter() const { return m_uiLastModificationCounter; }


private:
  ezUInt32 m_uiLastModificationCounter = 0;
  ezString m_sName;
  ezString m_sTargetPlatform = "Windows";
  ezDynamicArray<ezProfileConfigData*> m_Configs;
};

//////////////////////////////////////////////////////////////////////////

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Time/Timestamp.h>

struct ezPhantomRttiManagerEvent;
class ezExposedParameters;
struct ezAssetCuratorEvent;

/// \brief Lazily converts ezExposedParameters into phantom types.
/// Call GetExposedParametersType to create a type for a sub-asset ID.
class ezExposedParametersTypeRegistry
{
  EZ_DECLARE_SINGLETON(ezExposedParametersTypeRegistry);

public:
  ezExposedParametersTypeRegistry();
  ~ezExposedParametersTypeRegistry();
  /// \brief Returns null if the curator can find the asset or if the asset
  /// does not have any ezExposedParameters meta data.
  const ezRTTI* GetExposedParametersType(const char* szResource);
  /// \brief All exposed parameter types derive from this.
  const ezRTTI* GetExposedParametersBaseType() const { return m_pBaseType; }

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, ExposedParametersTypeRegistry);

  struct ParamData
  {
    ParamData() : m_pType(nullptr) {}

    ezUuid m_SubAssetGuid;
    bool m_bUpToDate = true;
    const ezRTTI* m_pType;
  };
  void UpdateExposedParametersType(ParamData& data, const ezExposedParameters& params);
  void AssetCuratorEventHandler(const ezAssetCuratorEvent& e);
  void PhantomTypeRegistryEventHandler(const ezPhantomRttiManagerEvent& e);

  ezMap<ezUuid, ParamData> m_ShaderTypes;
  const ezRTTI* m_pBaseType;
  ParamData* m_pAboutToBeRegistered = nullptr;
};

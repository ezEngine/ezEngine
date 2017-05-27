#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Algorithm/Hashing.h>

class ezRTTI;
class ezAbstractObjectNode;
class ezAbstractObjectGraph;
class ezGraphPatch;

/// \brief Singleton that allows version patching of ezAbstractObjectGraph.
///
/// Patching is automatically executed of ezAbstractObjectGraph de-serialize functions.
class EZ_FOUNDATION_DLL ezGraphVersioning
{
  EZ_DECLARE_SINGLETON(ezGraphVersioning);
public:
  ezGraphVersioning();
  ~ezGraphVersioning();

  /// \brief Patches all nodes inside pGraph to the current version.
  void PatchGraph(ezAbstractObjectGraph* pGraph);
  /// \brief Patches the type pType in pNode to version uiTypeVersion.
  void PatchNode(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode, const ezRTTI* pType, ezUInt32 uiTypeVersion);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, GraphVersioning);

  struct VersionKey
  {
    EZ_DECLARE_POD_TYPE();
    const ezRTTI* m_pType;
    ezUInt32 m_uiTypeVersion;
  };

  struct GraphVersioningHash
  {
    EZ_FORCE_INLINE static ezUInt32 Hash(const VersionKey& a)
    {
      ezUInt32 uiHash = ezHashing::MurmurHash(&a.m_pType, sizeof(a.m_pType));
      uiHash = ezHashing::MurmurHash(&a.m_uiTypeVersion, sizeof(a.m_uiTypeVersion), uiHash);
      return uiHash;
    }

    EZ_ALWAYS_INLINE static bool Equal(const VersionKey& a, const VersionKey& b)
    {
      return a.m_pType == b.m_pType && a.m_uiTypeVersion == b.m_uiTypeVersion;
    }
  };

  void PluginEventHandler(const ezPlugin::PluginEvent& EventData);
  void UpdatePatches();

  ezHashTable<VersionKey, const ezGraphPatch*, GraphVersioningHash> m_Patches;
};


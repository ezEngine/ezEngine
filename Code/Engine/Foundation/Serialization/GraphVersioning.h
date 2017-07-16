#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Algorithm/Hashing.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>

class ezRTTI;
class ezAbstractObjectNode;
class ezAbstractObjectGraph;
class ezGraphPatch;
class ezGraphPatchContext;

/// \brief Tuple used for identifying patches and tracking patch progression.
struct ezVersionKey
{
  EZ_DECLARE_POD_TYPE();
  ezHashedString m_sType;
  ezUInt32 m_uiTypeVersion;
};

/// \brief Hash helper class for ezVersionKey
struct ezGraphVersioningHash
{
  EZ_FORCE_INLINE static ezUInt32 Hash(const ezVersionKey& a)
  {
    ezUInt32 uiHash = a.m_sType.GetHash();
    uiHash = ezHashing::MurmurHash(&a.m_uiTypeVersion, sizeof(a.m_uiTypeVersion), uiHash);
    return uiHash;
  }

  EZ_ALWAYS_INLINE static bool Equal(const ezVersionKey& a, const ezVersionKey& b)
  {
    return a.m_sType == b.m_sType && a.m_uiTypeVersion == b.m_uiTypeVersion;
  }
};

/// \brief A class that overlaps ezReflectedTypeDescriptor with the properties needed for patching.
struct EZ_FOUNDATION_DLL ezTypeVersionInfo
{
  const char* GetTypeName() const;
  void SetTypeName(const char* szName);
  const char* GetParentTypeName() const;
  void SetParentTypeName(const char* szName);

  ezHashedString m_sTypeName;
  ezHashedString m_sParentTypeName;
  ezUInt32 m_uiTypeVersion;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezTypeVersionInfo);

/// \brief Handles the patching of a node. Is passed into the patch
///  classes to provide utility functions and track the node's patching progress.
class EZ_FOUNDATION_DLL ezGraphPatchContext
{
public:
  /// \brief Ensures that the base class named szType is at version uiTypeVersion.
  ///  If bForcePatch is set, the current version of the base class is reset back to force the execution
  ///  of this patch if necessary. This is mainly necessary for backwards compatibility with patches that
  ///  were written before the type information of all base classes was written to the doc.
  void PatchBaseClass(const char* szType, ezUInt32 uiTypeVersion, bool bForcePatch = false);

private:
  friend class ezGraphVersioning;
  ezGraphPatchContext(ezGraphVersioning* pParent, ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph);
  void Patch(ezAbstractObjectNode* pNode);
  void Patch(ezUInt32 uiBaseClassIndex, ezUInt32 uiTypeVersion, bool bForcePatch);
  void UpdateBaseClasses();

private:
  ezGraphVersioning* m_pParent = nullptr;
  ezAbstractObjectGraph* m_pGraph = nullptr;
  ezAbstractObjectNode* m_pNode = nullptr;
  ezDynamicArray<ezVersionKey> m_BaseClasses;
  ezUInt32 m_uiBaseClassIndex = 0;
  mutable ezHashTable<ezHashedString, ezTypeVersionInfo> m_TypeToInfo;
};

/// \brief Singleton that allows version patching of ezAbstractObjectGraph.
///
/// Patching is automatically executed of ezAbstractObjectGraph de-serialize functions.
class EZ_FOUNDATION_DLL ezGraphVersioning
{
  EZ_DECLARE_SINGLETON(ezGraphVersioning);
public:
  ezGraphVersioning();
  ~ezGraphVersioning();

  /// \brief Patches all nodes inside pGraph to the current version. pTypesGraph is the graph of serialized
  /// used types in pGraph at the time of saving. If not provided, any base class is assumed to be at max version.
  void PatchGraph(ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph = nullptr);

private:
  friend class ezGraphPatchContext;

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, GraphVersioning);

  void PluginEventHandler(const ezPlugin::PluginEvent& EventData);
  void UpdatePatches();
  ezUInt32 GetMaxPatchVersion(const ezHashedString& sType) const;

  ezHashTable<ezHashedString, ezUInt32> m_MaxPatchVersion; ///< Max version the given type can be patched to.
  ezDynamicArray<const ezGraphPatch*> m_GraphPatches;
  ezHashTable<ezVersionKey, const ezGraphPatch*, ezGraphVersioningHash> m_NodePatches;
};


#pragma once

/// \file

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Strings/HashedString.h>

class ezRTTI;
class ezAbstractObjectNode;
class ezAbstractObjectGraph;
class ezGraphPatch;
class ezGraphPatchContext;
class ezGraphVersioning;

/// \brief Identifier for graph patches combining type name and version number.
///
/// This structure uniquely identifies which patch should be applied to which type version.
/// The versioning system uses this to track patch progression and avoid duplicate applications.
struct ezVersionKey
{
  ezVersionKey() = default;
  ezVersionKey(ezStringView sType, ezUInt32 uiTypeVersion)
  {
    m_sType.Assign(sType);
    m_uiTypeVersion = uiTypeVersion;
  }
  EZ_DECLARE_POD_TYPE();
  ezHashedString m_sType;
  ezUInt32 m_uiTypeVersion;
};

/// \brief Hash helper class for ezVersionKey
struct ezGraphVersioningHash
{
  EZ_FORCE_INLINE static ezUInt32 Hash(const ezVersionKey& a)
  {
    auto typeNameHash = a.m_sType.GetHash();
    ezUInt32 uiHash = ezHashingUtils::xxHash32(&typeNameHash, sizeof(typeNameHash));
    uiHash = ezHashingUtils::xxHash32(&a.m_uiTypeVersion, sizeof(a.m_uiTypeVersion), uiHash);
    return uiHash;
  }

  EZ_ALWAYS_INLINE static bool Equal(const ezVersionKey& a, const ezVersionKey& b)
  {
    return a.m_sType == b.m_sType && a.m_uiTypeVersion == b.m_uiTypeVersion;
  }
};

/// \brief Stores type version information required for graph patching operations.
///
/// This structure contains the metadata needed to apply version patches, including
/// type names and version numbers for both the type and its parent class hierarchy.
/// It overlaps with ezReflectedTypeDescriptor to enable efficient patch processing.
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

/// \brief Context object that manages the patching process for individual nodes.
///
/// This class is passed to patch implementations to provide utility functions and track
/// the patching progress of a node. It handles base class patching, type renaming, and
/// hierarchy changes while maintaining consistency across the entire patching process.
class EZ_FOUNDATION_DLL ezGraphPatchContext
{
public:
  /// \brief Ensures a base class is patched to the specified version before continuing.
  ///
  /// This function forces the base class to be at the specified version, applying patches if necessary.
  /// Use bForcePatch for backwards compatibility when base class type information wasn't originally
  /// serialized. This ensures proper patch ordering in inheritance hierarchies.
  void PatchBaseClass(const char* szType, ezUInt32 uiTypeVersion, bool bForcePatch = false); // [tested]

  /// \brief Renames the current node's type to a new type name.
  ///
  /// Used when types are renamed or moved to different namespaces. The version number
  /// is preserved unless explicitly changed with the overload that takes a version parameter.
  void RenameClass(const char* szTypeName); // [tested]

  /// \brief Renames the current node's type and sets a new version number.
  ///
  /// Use this when both the type name and version change during a patch operation.
  /// This is common when types are refactored or split into multiple classes.
  void RenameClass(const char* szTypeName, ezUInt32 uiVersion);

  /// \brief Replaces the entire base class hierarchy with a new one.
  ///
  /// This is used for major refactoring where the inheritance structure changes.
  /// The array should contain the complete new inheritance chain from most derived
  /// to most base class. Handle with care as this affects serialization compatibility.
  void ChangeBaseClass(ezArrayPtr<ezVersionKey> baseClasses); // [tested]

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

/// \brief Singleton system that manages version patching for ezAbstractObjectGraph instances.
///
/// This system automatically applies version patches during deserialization to handle data migration
/// when type definitions change between versions. It supports both node-level patches (specific type
/// transformations) and graph-level patches (global transformations affecting multiple types).
///
/// The system automatically executes during ezAbstractObjectGraph deserialization,
/// ensuring that older serialized data can be loaded into newer application versions.
class EZ_FOUNDATION_DLL ezGraphVersioning
{
  EZ_DECLARE_SINGLETON(ezGraphVersioning);

public:
  ezGraphVersioning();
  ~ezGraphVersioning();

  /// \brief Applies all necessary patches to bring the graph to the current version.
  ///
  /// This is the main entry point for graph patching. It processes all nodes in the graph,
  /// applying patches in dependency order to ensure consistency.
  ///
  /// \param pGraph The object graph to patch (modified in-place)
  /// \param pTypesGraph Optional type information graph from serialization time.
  ///        Contains the exact type versions that were serialized. If not provided,
  ///        base classes are assumed to be at their maximum patchable version.
  ///
  /// The patching process:
  /// 1. Discovers all required patches for each node type
  /// 2. Sorts patches by dependency order (base classes first)
  /// 3. Applies patches incrementally until all nodes reach current versions
  /// 4. Validates that no circular dependencies exist
  void PatchGraph(ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph = nullptr);

private:
  friend class ezGraphPatchContext;

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, GraphVersioning);

  void PluginEventHandler(const ezPluginEvent& EventData);
  void UpdatePatches();
  ezUInt32 GetMaxPatchVersion(const ezHashedString& sType) const;

  ezHashTable<ezHashedString, ezUInt32> m_MaxPatchVersion; ///< Max version the given type can be patched to.
  ezDynamicArray<const ezGraphPatch*> m_GraphPatches;
  ezHashTable<ezVersionKey, const ezGraphPatch*, ezGraphVersioningHash> m_NodePatches;
};

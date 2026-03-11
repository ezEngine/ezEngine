#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Utilities/EnumerableClass.h>

class ezRTTI;
class ezAbstractObjectNode;
class ezAbstractObjectGraph;
class ezGraphVersioning;
class ezGraphPatchContext;

/// \brief Base class for implementing data migration patches for object graphs.
///
/// Graph patches enable automatic data migration when type definitions change between application
/// versions. By creating static instances of derived classes, patches are automatically registered
/// and applied during deserialization.
///
/// Patch implementation pattern:
/// 1. Create a class derived from ezGraphPatch
/// 2. Implement the Patch() method to transform data
/// 3. Create a static instance to auto-register the patch
/// 4. The versioning system automatically applies patches during load
///
/// Example:
/// \code
///   class MyTypePatch_1_to_2 : public ezGraphPatch
///   {
///   public:
///     MyTypePatch_1_to_2() : ezGraphPatch("MyType", 2) {}
///     virtual void Patch(ezGraphPatchContext& ctx, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
///     {
///       // Migrate data from version 1 to version 2
///       pNode->RenameProperty("oldName", "newName");
///     }
///   };
///   static MyTypePatch_1_to_2 g_MyTypePatch_1_to_2; // Auto-registers patch
/// \endcode
class EZ_FOUNDATION_DLL ezGraphPatch : public ezEnumerable<ezGraphPatch>
{
public:
  enum class PatchType : ezUInt8
  {
    NodePatch,  ///< Patch applies to individual nodes of a specific type and version
    GraphPatch, ///< Patch applies to the entire graph, processes all nodes regardless of type
  };

  /// \brief Constructs a patch for the specified type and target version.
  ///
  /// Parameters:
  /// - szType: Type name to patch (ignored for GraphPatch type)
  /// - uiTypeVersion: Target version to patch to
  /// - type: Whether this is a NodePatch (per-instance) or GraphPatch (global)
  ///
  /// Patch execution order:
  /// - Patches are applied incrementally from (uiTypeVersion-1) to uiTypeVersion
  /// - If gaps exist in patch versions, input may be older than (uiTypeVersion-1)
  /// - NodePatch: Applied once per instance of the specified type
  /// - GraphPatch: Applied once per graph, processes all relevant nodes internally
  ///
  /// For GraphPatch type, szType and uiTypeVersion are ignored during execution,
  /// and the patch implementation must determine what to process.
  ezGraphPatch(const char* szType, ezUInt32 uiTypeVersion, PatchType type = PatchType::NodePatch);

  /// \brief Main patch implementation - transforms data from old version to new version.
  ///
  /// Implementation requirements:
  /// - NodePatch: Transform pNode from its current version to m_uiTypeVersion
  /// - GraphPatch: Process entire pGraph, pNode will be nullptr
  ///
  /// Available operations through context:
  /// - Property manipulation (rename, add, remove, change type)
  /// - Type renaming and hierarchy changes
  ///
  /// Important: Patches should be idempotent and handle missing properties gracefully
  /// for robustness against incomplete version chains.
  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const = 0;
  /// \brief Returns the type to patch.
  const char* GetType() const;
  /// \brief Returns the type version to patch to.
  ezUInt32 GetTypeVersion() const;
  PatchType GetPatchType() const;

  EZ_DECLARE_ENUMERABLE_CLASS(ezGraphPatch);

private:
  const char* m_szType = nullptr;
  ezUInt32 m_uiTypeVersion;
  PatchType m_PatchType;
};

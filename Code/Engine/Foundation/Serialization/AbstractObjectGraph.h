#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>

class ezAbstractObjectGraph;

class EZ_FOUNDATION_DLL ezAbstractObjectNode
{
public:
  struct Property
  {
    ezStringView m_sPropertyName;
    ezVariant m_Value;
  };

  ezAbstractObjectNode() = default;

  const ezHybridArray<Property, 16>& GetProperties() const { return m_Properties; }

  void AddProperty(ezStringView sName, const ezVariant& value);

  void RemoveProperty(ezStringView sName);

  void ChangeProperty(ezStringView sName, const ezVariant& value);

  void RenameProperty(ezStringView sOldName, ezStringView sNewName);

  void ClearProperties();

  // \brief Inlines a custom variant type. Use to patch properties that have been turned into custom variant type.
  // \sa EZ_DEFINE_CUSTOM_VARIANT_TYPE, EZ_DECLARE_CUSTOM_VARIANT_TYPE
  ezResult InlineProperty(ezStringView sName);

  const ezAbstractObjectGraph* GetOwner() const { return m_pOwner; }
  const ezUuid& GetGuid() const { return m_Guid; }
  ezUInt32 GetTypeVersion() const { return m_uiTypeVersion; }
  void SetTypeVersion(ezUInt32 uiTypeVersion) { m_uiTypeVersion = uiTypeVersion; }
  ezStringView GetType() const { return m_sType; }
  void SetType(ezStringView sType);

  const Property* FindProperty(ezStringView sName) const;
  Property* FindProperty(ezStringView sName);

  ezStringView GetNodeName() const { return m_sNodeName; }

private:
  friend class ezAbstractObjectGraph;

  ezAbstractObjectGraph* m_pOwner = nullptr;

  ezUuid m_Guid;
  ezUInt32 m_uiTypeVersion = 0;
  ezStringView m_sType;
  ezStringView m_sNodeName;

  ezHybridArray<Property, 16> m_Properties;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezAbstractObjectNode);

struct EZ_FOUNDATION_DLL ezAbstractGraphDiffOperation
{
  enum class Op
  {
    NodeAdded,
    NodeRemoved,
    PropertyChanged
  };

  Op m_Operation;
  ezUuid m_Node;            // prop parent or added / deleted node
  ezString m_sProperty;     // prop name or type
  ezUInt32 m_uiTypeVersion; // only used for NodeAdded
  ezVariant m_Value;
};

struct EZ_FOUNDATION_DLL ezObjectChangeType
{
  using StorageType = ezInt8;

  enum Enum : ezInt8
  {
    NodeAdded,
    NodeRemoved,
    PropertySet,
    PropertyInserted,
    PropertyRemoved,

    Default = NodeAdded
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezObjectChangeType);


struct EZ_FOUNDATION_DLL ezDiffOperation
{
  ezEnum<ezObjectChangeType> m_Operation;
  ezUuid m_Node;        // owner of m_sProperty
  ezString m_sProperty; // property
  ezVariant m_Index;
  ezVariant m_Value;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezDiffOperation);


/// \brief An intermediate representation for serializing/deserializing reflected objects.
///
/// The AbstractObjectGraph represents objects and their properties in a type-independent way,
/// allowing for versioning, patching, and format conversion. It serves as the core of ezEngine's
/// serialization system and enables advanced features like:
///
/// - Cross-format serialization (DDL, binary, JSON)
/// - Type versioning and migration
/// - Graph diffing and merging
/// - Prefab instantiation and overrides
/// - Undo/redo systems
///
/// Structure:
/// - Graph contains nodes (ezAbstractObjectNode) identified by GUIDs
/// - Each node has a type name, version, and properties
/// - Properties store values as ezVariant (including object references)
/// - References between objects are stored as GUIDs
class EZ_FOUNDATION_DLL ezAbstractObjectGraph
{
public:
  ezAbstractObjectGraph() = default;
  ~ezAbstractObjectGraph();

  void Clear();

  using FilterFunction = ezDelegate<bool(const ezAbstractObjectNode*, const ezAbstractObjectNode::Property*)>;
  ezAbstractObjectNode* Clone(ezAbstractObjectGraph& ref_cloneTarget, const ezAbstractObjectNode* pRootNode = nullptr, FilterFunction filter = FilterFunction()) const;

  ezStringView RegisterString(ezStringView sString);

  const ezAbstractObjectNode* GetNode(const ezUuid& guid) const;
  ezAbstractObjectNode* GetNode(const ezUuid& guid);

  const ezAbstractObjectNode* GetNodeByName(ezStringView sName) const;
  ezAbstractObjectNode* GetNodeByName(ezStringView sName);

  ezAbstractObjectNode* AddNode(const ezUuid& guid, ezStringView sType, ezUInt32 uiTypeVersion, ezStringView sNodeName = {});
  void RemoveNode(const ezUuid& guid);

  const ezMap<ezUuid, ezAbstractObjectNode*>& GetAllNodes() const { return m_Nodes; }
  ezMap<ezUuid, ezAbstractObjectNode*>& GetAllNodes() { return m_Nodes; }

  /// \brief Remaps all node guids by adding the given seed, or if bRemapInverse is true, by subtracting it/
  ///   This is mostly used to remap prefab instance graphs to their prefab template graph.
  void ReMapNodeGuids(const ezUuid& seedGuid, bool bRemapInverse = false);

  /// \brief Tries to remap the guids of this graph to those in rhsGraph by walking in both down the hierarchy, starting at root and
  /// rhsRoot.
  ///
  ///  Note that in case of array properties the remapping assumes element indices to be equal
  ///  on both sides which will cause all moves inside the arrays to be lost as there is no way of recovering this information without an
  ///  equality criteria. This function is mostly used to remap a graph from a native object to a graph from ezDocumentObjects to allow
  ///  applying native side changes to the original ezDocumentObject hierarchy using diffs.
  void ReMapNodeGuidsToMatchGraph(ezAbstractObjectNode* pRoot, const ezAbstractObjectGraph& rhsGraph, const ezAbstractObjectNode* pRhsRoot);

  /// \brief Finds everything accessible by the given root node.
  void FindTransitiveHull(const ezUuid& rootGuid, ezSet<ezUuid>& out_reachableNodes) const;
  /// \brief Deletes everything not accessible by the given root node.
  void PruneGraph(const ezUuid& rootGuid);

  /// \brief Allows a node to be modified as a native object and automatically syncs changes back.
  ///
  /// This temporarily converts the node (and its sub-hierarchy) to native objects, calls the provided
  /// callback to allow modifications, then converts the modified objects back to the graph representation.
  /// This is useful for applying complex modifications that are easier to implement on native objects.
  /// Changes to the object hierarchy (adding/removing children) will be reflected in the graph.
  void ModifyNodeViaNativeCounterpart(ezAbstractObjectNode* pRootNode, ezDelegate<void(void*, const ezRTTI*)> callback);

  /// \brief Allows to copy a node from another graph into this graph.
  ezAbstractObjectNode* CopyNodeIntoGraph(const ezAbstractObjectNode* pNode);

  ezAbstractObjectNode* CopyNodeIntoGraph(const ezAbstractObjectNode* pNode, FilterFunction& ref_filter);

  void CreateDiffWithBaseGraph(const ezAbstractObjectGraph& base, ezDeque<ezAbstractGraphDiffOperation>& out_diffResult) const;

  void ApplyDiff(ezDeque<ezAbstractGraphDiffOperation>& ref_diff);

  void MergeDiffs(const ezDeque<ezAbstractGraphDiffOperation>& lhs, const ezDeque<ezAbstractGraphDiffOperation>& rhs, ezDeque<ezAbstractGraphDiffOperation>& ref_out) const;

private:
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAbstractObjectGraph);

  void RemapVariant(ezVariant& value, const ezHashTable<ezUuid, ezUuid>& guidMap);
  void MergeArrays(const ezVariantArray& baseArray, const ezVariantArray& leftArray, const ezVariantArray& rightArray, ezVariantArray& out) const;
  void ReMapNodeGuidsToMatchGraphRecursive(ezHashTable<ezUuid, ezUuid>& guidMap, ezAbstractObjectNode* lhs, const ezAbstractObjectGraph& rhsGraph, const ezAbstractObjectNode* rhs);

  ezSet<ezString> m_Strings;
  ezMap<ezUuid, ezAbstractObjectNode*> m_Nodes;
  ezMap<ezStringView, ezAbstractObjectNode*> m_NodesByName;
};

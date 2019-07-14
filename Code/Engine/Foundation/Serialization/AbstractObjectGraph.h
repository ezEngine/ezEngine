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
    const char* m_szPropertyName;
    ezVariant m_Value;
  };

  ezAbstractObjectNode()
      : m_pOwner(nullptr)
      , m_uiTypeVersion(0)
      , m_szType(nullptr)
      , m_szNodeName(nullptr)
  {
  }

  const ezHybridArray<Property, 16>& GetProperties() const { return m_Properties; }

  void AddProperty(const char* szName, const ezVariant& value);

  void RemoveProperty(const char* szName);

  void ChangeProperty(const char* szName, const ezVariant& value);

  void RenameProperty(const char* szOldName, const char* szNewName);

  const ezUuid& GetGuid() const { return m_Guid; }
  const ezUInt32 GetTypeVersion() const { return m_uiTypeVersion; }
  void SetTypeVersion(ezUInt32 uiTypeVersion) { m_uiTypeVersion = uiTypeVersion; }
  const char* GetType() const { return m_szType; }
  void SetType(const char* szType);

  const Property* FindProperty(const char* szName) const;
  Property* FindProperty(const char* szName);

  const char* GetNodeName() const { return m_szNodeName; }

private:
  friend class ezAbstractObjectGraph;

  ezAbstractObjectGraph* m_pOwner;

  ezUuid m_Guid;
  ezUInt32 m_uiTypeVersion;
  const char* m_szType;
  const char* m_szNodeName;

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
  typedef ezInt8 StorageType;

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


class EZ_FOUNDATION_DLL ezAbstractObjectGraph
{
public:
  ezAbstractObjectGraph() {}
  ~ezAbstractObjectGraph();

  void Clear();
  void Clone(ezAbstractObjectGraph& cloneTarget) const;

  const char* RegisterString(const char* szString);

  const ezAbstractObjectNode* GetNode(const ezUuid& guid) const;
  ezAbstractObjectNode* GetNode(const ezUuid& guid);

  const ezAbstractObjectNode* GetNodeByName(const char* szName) const;
  ezAbstractObjectNode* GetNodeByName(const char* szName);

  ezAbstractObjectNode* AddNode(const ezUuid& guid, const char* szType, ezUInt32 uiTypeVersion, const char* szNodeName = nullptr);
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
  void ReMapNodeGuidsToMatchGraph(ezAbstractObjectNode* root, const ezAbstractObjectGraph& rhsGraph, const ezAbstractObjectNode* rhsRoot);

  /// \brief Deletes everything not accessible by the given root node.
  void PruneGraph(const ezUuid& rootGuid);

  /// \brief Allows to copy a node from another graph into this graph.
  ezAbstractObjectNode* CopyNodeIntoGraph(const ezAbstractObjectNode* pNode);

  void CreateDiffWithBaseGraph(const ezAbstractObjectGraph& base, ezDeque<ezAbstractGraphDiffOperation>& out_DiffResult) const;

  void ApplyDiff(ezDeque<ezAbstractGraphDiffOperation>& Diff);

  void MergeDiffs(const ezDeque<ezAbstractGraphDiffOperation>& lhs, const ezDeque<ezAbstractGraphDiffOperation>& rhs,
                  ezDeque<ezAbstractGraphDiffOperation>& out) const;

private:
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAbstractObjectGraph);

  void RemapVariant(ezVariant& value, const ezHashTable<ezUuid, ezUuid>& guidMap);
  void MergeArrays(const ezVariantArray& baseArray, const ezVariantArray& leftArray, const ezVariantArray& rightArray,
                   ezVariantArray& out) const;
  void ReMapNodeGuidsToMatchGraphRecursive(ezHashTable<ezUuid, ezUuid>& guidMap, ezAbstractObjectNode* lhs, const ezAbstractObjectGraph& rhsGraph,
                                           const ezAbstractObjectNode* rhs);

  ezSet<ezString> m_Strings;
  ezMap<ezUuid, ezAbstractObjectNode*> m_Nodes;
  ezMap<const char*, ezAbstractObjectNode*, CompareConstChar> m_NodesByName;
};


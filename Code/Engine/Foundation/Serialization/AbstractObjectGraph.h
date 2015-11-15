#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Set.h>

class ezAbstractObjectGraph;

class EZ_FOUNDATION_DLL ezAbstractObjectNode
{
public:
  struct Property
  {
    const char* m_szPropertyName;
    ezVariant m_Value;
  };

  const ezHybridArray<Property, 16>& GetProperties() const { return m_Properties; }

  void AddProperty(const char* szName, const ezVariant& value);

  void RemoveProperty(const char* szName);

  void ChangeProperty(const char* szName, const ezVariant& value);

  const ezUuid& GetGuid() const { return m_Guid; }
  const char* GetType() const { return m_szType; }

  const Property* FindProperty(const char* szName) const;
  Property* FindProperty(const char* szName);

  const char* GetNodeName() const { return m_szNodeName; }

private:
  friend class ezAbstractObjectGraph;

  ezAbstractObjectGraph* m_pOwner;

  ezUuid m_Guid;
  const char* m_szType;
  const char* m_szNodeName;

  ezHybridArray<Property, 16> m_Properties;
};

class EZ_FOUNDATION_DLL ezAbstractGraphDiffOperation
{
public:
  enum class Op
  {
    NodeAdd,
    NodeDelete,
    PropertySet,
  };

  Op m_Operation;
  ezUuid m_Node;
  ezString m_sProperty;
  ezVariant m_Value;
};

class EZ_FOUNDATION_DLL ezAbstractObjectGraph
{
public:
  ezAbstractObjectGraph() {}
  ~ezAbstractObjectGraph();

  void Clear();

  const char* RegisterString(const char* szString);

  const ezAbstractObjectNode* GetNode(const ezUuid& guid) const;
  ezAbstractObjectNode* GetNode(const ezUuid& guid);

  const ezAbstractObjectNode* GetNodeByName(const char* szName) const;
  ezAbstractObjectNode* GetNodeByName(const char* szName);

  ezAbstractObjectNode* AddNode(const ezUuid& guid, const char* szType, const char* szNodeName = nullptr);
  void RemoveNode(const ezUuid& guid);

  const ezMap<ezUuid, ezAbstractObjectNode*>& GetAllNodes() const { return m_Nodes; }
  ezMap<ezUuid, ezAbstractObjectNode*>& GetAllNodes() { return m_Nodes; }

  void ReMapNodeGuids(const ezUuid& seedGuid, bool bRemapInverse = false);

  /// \brief Allows to copy a node from another graph into this graph.
  void CopyNodeIntoGraph(ezAbstractObjectNode* pNode);

  void CreateDiffWithBaseGraph(const ezAbstractObjectGraph& base, ezDeque<ezAbstractGraphDiffOperation>& out_DiffResult);

  void ApplyDiff(ezDeque<ezAbstractGraphDiffOperation>& Diff);

private:
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAbstractObjectGraph);
  void RemapVariant(ezVariant& value, const ezMap<ezUuid, ezUuid>& guidMap);

  ezSet<ezString> m_Strings;
  ezMap<ezUuid, ezAbstractObjectNode*> m_Nodes;
  ezMap<const char*, ezAbstractObjectNode*> m_NodesByName;
};


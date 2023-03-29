#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class ezDocumentObject;

class EZ_TOOLSFOUNDATION_DLL ezPrefabUtils
{
public:
  /// \brief
  static void LoadGraph(ezAbstractObjectGraph& out_graph, const char* szGraph);

  static ezAbstractObjectNode* GetFirstRootNode(ezAbstractObjectGraph& ref_graph);

  static void GetRootNodes(ezAbstractObjectGraph& ref_graph, ezHybridArray<ezAbstractObjectNode*, 4>& out_nodes);

  static ezUuid GetPrefabRoot(const ezDocumentObject* pObject, const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>& documentObjectMetaData, ezInt32* pDepth = nullptr);

  static ezVariant GetDefaultValue(
    const ezAbstractObjectGraph& graph, const ezUuid& objectGuid, const char* szProperty, ezVariant index = ezVariant(), bool* pValueFound = nullptr);

  static void WriteDiff(const ezDeque<ezAbstractGraphDiffOperation>& mergedDiff, ezStringBuilder& out_sText);

  /// \brief Merges diffs of left and right graphs relative to their base graph. Conflicts prefer the right graph.
  static void Merge(const ezAbstractObjectGraph& baseGraph, const ezAbstractObjectGraph& leftGraph, const ezAbstractObjectGraph& rightGraph,
    ezDeque<ezAbstractGraphDiffOperation>& out_mergedDiff);

  /// \brief Merges diffs of left and right graphs relative to their base graph. Conflicts prefer the right graph. Base and left are provided as
  /// serialized DDL graphs and the right graph is build directly from pRight and its PrefabSeed.
  static void Merge(const char* szBase, const char* szLeft, ezDocumentObject* pRight, bool bRightIsNotPartOfPrefab, const ezUuid& prefabSeed,
    ezStringBuilder& out_sNewGraph);

  static ezString ReadDocumentAsString(const char* szFile);
};

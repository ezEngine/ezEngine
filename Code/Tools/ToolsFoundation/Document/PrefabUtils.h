#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <CoreUtils/DataStructures/ObjectMetaData.h>
#include <ToolsFoundation/Document/Document.h>

class ezDocumentObject;

class EZ_TOOLSFOUNDATION_DLL ezPrefabUtils
{
public:
  /// \brief 
  static void LoadGraph(ezAbstractObjectGraph& out_graph, const char* szGraph);

  static ezAbstractObjectNode* GetFirstRootNode(ezAbstractObjectGraph& graph);

  static ezUuid GetPrefabRoot(const ezDocumentObject* pObject, const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>& documentObjectMetaData);

  static ezVariant GetDefaultValue(const ezAbstractObjectGraph& graph, const ezUuid& objectGuid, const ezPropertyPath& path);

  static void WriteDiff(const ezDeque<ezAbstractGraphDiffOperation>& mergedDiff, ezStringBuilder& out_sText);

  /// \brief Merges diffs of left and right graphs relative to their base graph. Conflicts prefer the right graph. 
  static void Merge(const ezAbstractObjectGraph& baseGraph, const ezAbstractObjectGraph& leftGraph, const ezAbstractObjectGraph& rightGraph, ezDeque<ezAbstractGraphDiffOperation>& out_mergedDiff);

  /// \brief Merges diffs of left and right graphs relative to their base graph. Conflicts prefer the right graph. Base and left are provided as serialized json graphs
  /// and the right graph is build directly from pRight and its PrefabSeed.
  static void Merge(const char* szBase, const char* szLeft, ezDocumentObject* pRight, const ezUuid& PrefabSeed, ezStringBuilder& out_sNewGraph);

};


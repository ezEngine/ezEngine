#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

#define PREFAB_DEBUG false

ezString ToBinary(const ezUuid& guid)
{
  ezStringBuilder s, sResult;

  ezUInt8* pBytes = (ezUInt8*)&guid;

  for (ezUInt32 i = 0; i < sizeof(ezUuid); ++i)
  {
    s.SetFormat("{0}", ezArgU((ezUInt32)*pBytes, 2, true, 16, true));
    ++pBytes;

    sResult.Append(s.GetData());
  }

  return sResult;
}

void ezPrefabUtils::LoadGraph(ezAbstractObjectGraph& out_graph, ezStringView sGraph)
{
  ezPrefabCache::GetSingleton()->LoadGraph(out_graph, ezStringView(sGraph));
}


ezAbstractObjectNode* ezPrefabUtils::GetFirstRootNode(ezAbstractObjectGraph& ref_graph)
{
  auto& nodes = ref_graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    if (pNode->GetNodeName() == "ObjectTree")
    {
      for (const auto& ObjectTreeProp : pNode->GetProperties())
      {
        if (ObjectTreeProp.m_sPropertyName == "Children" && ObjectTreeProp.m_Value.IsA<ezVariantArray>())
        {
          const ezVariantArray& RootChildren = ObjectTreeProp.m_Value.Get<ezVariantArray>();

          for (const ezVariant& childGuid : RootChildren)
          {
            if (!childGuid.IsA<ezUuid>())
              continue;

            const ezUuid& rootObjectGuid = childGuid.Get<ezUuid>();

            return ref_graph.GetNode(rootObjectGuid);
          }
        }
      }
    }
  }
  return nullptr;
}

void ezPrefabUtils::GetRootNodes(ezAbstractObjectGraph& ref_graph, ezHybridArray<ezAbstractObjectNode*, 4>& out_nodes)
{
  auto& nodes = ref_graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    if (pNode->GetNodeName() == "ObjectTree")
    {
      for (const auto& ObjectTreeProp : pNode->GetProperties())
      {
        if (ObjectTreeProp.m_sPropertyName == "Children" && ObjectTreeProp.m_Value.IsA<ezVariantArray>())
        {
          const ezVariantArray& RootChildren = ObjectTreeProp.m_Value.Get<ezVariantArray>();

          for (const ezVariant& childGuid : RootChildren)
          {
            if (!childGuid.IsA<ezUuid>())
              continue;

            const ezUuid& rootObjectGuid = childGuid.Get<ezUuid>();

            out_nodes.PushBack(ref_graph.GetNode(rootObjectGuid));
          }

          return;
        }
      }

      return;
    }
  }
}

ezUuid ezPrefabUtils::GetPrefabRoot(const ezDocumentObject* pObject, const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>& documentObjectMetaData, ezInt32* pDepth)
{
  auto pMeta = documentObjectMetaData.BeginReadMetaData(pObject->GetGuid());
  ezUuid source = pMeta->m_CreateFromPrefab;
  documentObjectMetaData.EndReadMetaData();

  if (source.IsValid())
  {
    return pObject->GetGuid();
  }

  if (pObject->GetParent() != nullptr)
  {
    if (pDepth)
      *pDepth += 1;
    return GetPrefabRoot(pObject->GetParent(), documentObjectMetaData);
  }
  return ezUuid();
}


ezVariant ezPrefabUtils::GetDefaultValue(const ezAbstractObjectGraph& graph, const ezUuid& objectGuid, ezStringView sProperty, ezVariant index, bool* pValueFound)
{
  if (pValueFound)
    *pValueFound = false;

  const ezAbstractObjectNode* pNode = graph.GetNode(objectGuid);
  if (!pNode)
    return ezVariant();

  const ezAbstractObjectNode::Property* pProp = pNode->FindProperty(sProperty);
  if (pProp)
  {
    const ezVariant& value = pProp->m_Value;

    if (value.IsA<ezVariantArray>() && index.CanConvertTo<ezUInt32>())
    {
      ezUInt32 uiIndex = index.ConvertTo<ezUInt32>();
      const ezVariantArray& valueArray = value.Get<ezVariantArray>();
      if (uiIndex < valueArray.GetCount())
      {
        if (pValueFound)
          *pValueFound = true;
        return valueArray[uiIndex];
      }
      return ezVariant();
    }
    else if (value.IsA<ezVariantDictionary>() && index.CanConvertTo<ezString>())
    {
      ezString sKey = index.ConvertTo<ezString>();
      const ezVariantDictionary& valueDict = value.Get<ezVariantDictionary>();
      auto it = valueDict.Find(sKey);
      if (it.IsValid())
      {
        if (pValueFound)
          *pValueFound = true;
        return it.Value();
      }
      return ezVariant();
    }
    if (pValueFound)
      *pValueFound = true;
    return value;
  }

  return ezVariant();
}

void ezPrefabUtils::WriteDiff(const ezDeque<ezAbstractGraphDiffOperation>& mergedDiff, ezStringBuilder& out_sText)
{
  for (const auto& diff : mergedDiff)
  {
    ezStringBuilder Data = ToBinary(diff.m_Node);

    switch (diff.m_Operation)
    {
      case ezAbstractGraphDiffOperation::Op::NodeAdded:
      {
        out_sText.AppendFormat("<add> - {{0}} ({1})\n", Data, diff.m_sProperty);
      }
      break;

      case ezAbstractGraphDiffOperation::Op::NodeRemoved:
      {
        out_sText.AppendFormat("<del> - {{0}}\n", Data);
      }
      break;

      case ezAbstractGraphDiffOperation::Op::PropertyChanged:
        if (diff.m_Value.CanConvertTo<ezString>())
          out_sText.AppendFormat("<set> - {{0}} - \"{1}\" = {2}\n", Data, diff.m_sProperty, diff.m_Value.ConvertTo<ezString>());
        else
          out_sText.AppendFormat("<set> - {{0}} - \"{1}\" = xxx\n", Data, diff.m_sProperty);
        break;
    }
  }
}

void ezPrefabUtils::Merge(const ezAbstractObjectGraph& baseGraph, const ezAbstractObjectGraph& leftGraph, const ezAbstractObjectGraph& rightGraph, ezDeque<ezAbstractGraphDiffOperation>& out_mergedDiff)
{
  // debug output
  if (PREFAB_DEBUG)
  {
    {
      ezFileWriter file;
      file.Open("C:\\temp\\Prefab - base.txt").IgnoreResult();
      ezAbstractGraphDdlSerializer::Write(file, &baseGraph, nullptr, false, ezOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }

    {
      ezFileWriter file;
      file.Open("C:\\temp\\Prefab - template.txt").IgnoreResult();
      ezAbstractGraphDdlSerializer::Write(file, &leftGraph, nullptr, false, ezOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }

    {
      ezFileWriter file;
      file.Open("C:\\temp\\Prefab - instance.txt").IgnoreResult();
      ezAbstractGraphDdlSerializer::Write(file, &rightGraph, nullptr, false, ezOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }
  }

  ezDeque<ezAbstractGraphDiffOperation> LeftToBase;
  leftGraph.CreateDiffWithBaseGraph(baseGraph, LeftToBase);
  ezDeque<ezAbstractGraphDiffOperation> RightToBase;
  rightGraph.CreateDiffWithBaseGraph(baseGraph, RightToBase);

  baseGraph.MergeDiffs(LeftToBase, RightToBase, out_mergedDiff);

  // debug output
  if (PREFAB_DEBUG)
  {
    ezFileWriter file;
    file.Open("C:\\temp\\Prefab - diff.txt").IgnoreResult();

    ezStringBuilder sDiff;
    sDiff.Append("######## Template To Base #######\n");
    ezPrefabUtils::WriteDiff(LeftToBase, sDiff);
    sDiff.Append("\n\n######## Instance To Base #######\n");
    ezPrefabUtils::WriteDiff(RightToBase, sDiff);
    sDiff.Append("\n\n######## Merged Diff #######\n");
    ezPrefabUtils::WriteDiff(out_mergedDiff, sDiff);


    file.WriteBytes(sDiff.GetData(), sDiff.GetElementCount()).IgnoreResult();
  }
}

void ezPrefabUtils::Merge(ezStringView sBase, ezStringView sLeft, ezDocumentObject* pRight, bool bRightIsNotPartOfPrefab, const ezUuid& prefabSeed, ezStringBuilder& out_sNewGraph)
{
  // prepare the original prefab as a graph
  ezAbstractObjectGraph baseGraph;
  ezPrefabUtils::LoadGraph(baseGraph, sBase);
  if (auto pHeader = baseGraph.GetNodeByName("Header"))
  {
    baseGraph.RemoveNode(pHeader->GetGuid());
  }

  {
    // read the new template as a graph
    ezAbstractObjectGraph leftGraph;
    ezPrefabUtils::LoadGraph(leftGraph, sLeft);
    if (auto pHeader = leftGraph.GetNodeByName("Header"))
    {
      leftGraph.RemoveNode(pHeader->GetGuid());
    }

    // prepare the current state as a graph
    ezAbstractObjectGraph rightGraph;
    {
      ezDocumentObjectConverterWriter writer(&rightGraph, pRight->GetDocumentObjectManager());

      ezVariantArray children;
      if (bRightIsNotPartOfPrefab)
      {
        for (ezDocumentObject* pChild : pRight->GetChildren())
        {
          writer.AddObjectToGraph(pChild);
          children.PushBack(pChild->GetGuid());
        }
      }
      else
      {
        writer.AddObjectToGraph(pRight);
        children.PushBack(pRight->GetGuid());
      }

      rightGraph.ReMapNodeGuids(prefabSeed, true);
      // just take the entire ObjectTree node as is TODO: this may cause a crash if the root object is replaced
      ezAbstractObjectNode* pRightObjectTree = rightGraph.CopyNodeIntoGraph(leftGraph.GetNodeByName("ObjectTree"));
      // The root node should always have a property 'children' where all the root objects are attached to. We need to replace that property's value as the prefab instance graph can have less or more objects than the template.
      ezAbstractObjectNode::Property* pChildrenProp = pRightObjectTree->FindProperty("Children");
      pChildrenProp->m_Value = children;
    }

    // Merge diffs relative to base
    ezDeque<ezAbstractGraphDiffOperation> mergedDiff;
    ezPrefabUtils::Merge(baseGraph, leftGraph, rightGraph, mergedDiff);


    {
      // Apply merged diff to base.
      baseGraph.ApplyDiff(mergedDiff);

      ezContiguousMemoryStreamStorage stor;
      ezMemoryStreamWriter sw(&stor);

      ezAbstractGraphDdlSerializer::Write(sw, &baseGraph, nullptr, true, ezOpenDdlWriter::TypeStringMode::Shortest);

      out_sNewGraph.SetSubString_ElementCount((const char*)stor.GetData(), stor.GetStorageSize32());
    }

    // debug output
    if (PREFAB_DEBUG)
    {
      ezFileWriter file;
      file.Open("C:\\temp\\Prefab - result.txt").IgnoreResult();
      ezAbstractGraphDdlSerializer::Write(file, &baseGraph, nullptr, false, ezOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }
  }
}

ezString ezPrefabUtils::ReadDocumentAsString(ezStringView sFile)
{
  ezFileReader file;
  if (file.Open(sFile) == EZ_FAILURE)
  {
    ezLog::Error("Failed to open document file '{0}'", sFile);
    return ezString();
  }

  ezStringBuilder sGraph;
  sGraph.ReadAll(file);

  return sGraph;
}

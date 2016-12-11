#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Serialization/DdlSerializer.h>

#define PREFAB_DEBUG false

ezString ToBinary(const ezUuid& guid)
{
  ezStringBuilder s, sResult;

  ezUInt8* pBytes = (ezUInt8*)&guid;

  for (ezUInt32 i = 0; i < sizeof(ezUuid); ++i)
  {
    s.Format("{0}", ezArgU((ezUInt32)*pBytes, 2, true, 16, true));
    ++pBytes;

    sResult.Append(s.GetData());
  }

  return sResult;
}

void ezPrefabUtils::LoadGraph(ezAbstractObjectGraph& out_graph, const char* szGraph)
{
  ezPrefabCache::GetSingleton()->LoadGraph(out_graph, ezStringView(szGraph));
}


ezAbstractObjectNode* ezPrefabUtils::GetFirstRootNode(ezAbstractObjectGraph& graph)
{
  auto& nodes = graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    if (ezStringUtils::IsEqual(pNode->GetNodeName(), "ObjectTree"))
    {
      for (const auto& ObjectTreeProp : pNode->GetProperties())
      {
        if (ezStringUtils::IsEqual(ObjectTreeProp.m_szPropertyName, "Children") && ObjectTreeProp.m_Value.IsA<ezVariantArray>())
        {
          const ezVariantArray& RootChildren = ObjectTreeProp.m_Value.Get<ezVariantArray>();

          for (const ezVariant& childGuid : RootChildren)
          {
            if (!childGuid.IsA<ezUuid>())
              continue;

            const ezUuid& rootObjectGuid = childGuid.Get<ezUuid>();

            return graph.GetNode(rootObjectGuid);
          }
        }
      }
    }
  }
  return nullptr;
}


ezUuid ezPrefabUtils::GetPrefabRoot(const ezDocumentObject* pObject, const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>& documentObjectMetaData)
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
    return GetPrefabRoot(pObject->GetParent(), documentObjectMetaData);
  }
  return ezUuid();
}


ezVariant ezPrefabUtils::GetDefaultValue(const ezAbstractObjectGraph& graph, const ezUuid& objectGuid, const char* szProperty, ezVariant index)
{
  const ezAbstractObjectNode* pNode = graph.GetNode(objectGuid);
  ezUInt32 uiIndex = 0;
  const ezAbstractObjectNode::Property* pProp = pNode->FindProperty(szProperty);
  if (pProp)
  {
    const ezVariant& value = pProp->m_Value;

    if (value.IsA<ezVariantArray>() && index.CanConvertTo<ezUInt32>())
    {
      ezUInt32 uiIndex = index.ConvertTo<ezUInt32>();
      const ezVariantArray& valueArray = value.Get<ezVariantArray>();
      if (uiIndex < valueArray.GetCount())
      {
        return valueArray[uiIndex];
      }
      return ezVariant();
    }
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
        out_sText.AppendFormat("<add> - {{0}} ({1})\n", Data.GetData(), diff.m_sProperty.GetData());
      }
      break;

    case ezAbstractGraphDiffOperation::Op::NodeRemoved:
      {
        out_sText.AppendFormat("<del> - {{0}}\n", Data.GetData());
      }
      break;

    case ezAbstractGraphDiffOperation::Op::PropertyChanged:
      if (diff.m_Value.CanConvertTo<ezString>())
        out_sText.AppendFormat("<set> - {{0}} - \"{1}\" = {2}\n", Data.GetData(), diff.m_sProperty.GetData(), diff.m_Value.ConvertTo<ezString>().GetData());
      else
        out_sText.AppendFormat("<set> - {{0}} - \"{1}\" = xxx\n", Data.GetData(), diff.m_sProperty.GetData());
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
      file.Open("C:\\temp\\Prefab - base.txt");
      ezAbstractGraphDdlSerializer::Write(file, &baseGraph, nullptr, false, ezOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }

    {
      ezFileWriter file;
      file.Open("C:\\temp\\Prefab - template.txt");
      ezAbstractGraphDdlSerializer::Write(file, &leftGraph, nullptr, false, ezOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }

    {
      ezFileWriter file;
      file.Open("C:\\temp\\Prefab - instance.txt");
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
    file.Open("C:\\temp\\Prefab - diff.txt");

    ezStringBuilder sDiff;
    sDiff.Append("######## Template To Base #######\n");
    ezPrefabUtils::WriteDiff(LeftToBase, sDiff);
    sDiff.Append("\n\n######## Instance To Base #######\n");
    ezPrefabUtils::WriteDiff(RightToBase, sDiff);
    sDiff.Append("\n\n######## Merged Diff #######\n");
    ezPrefabUtils::WriteDiff(out_mergedDiff, sDiff);


    file.WriteBytes(sDiff.GetData(), sDiff.GetElementCount());
  }
}

void ezPrefabUtils::Merge(const char* szBase, const char* szLeft, ezDocumentObject* pRight, const ezUuid& PrefabSeed, ezStringBuilder& out_sNewGraph)
{
  // prepare the original prefab as a graph
  ezAbstractObjectGraph baseGraph;
  ezPrefabUtils::LoadGraph(baseGraph, szBase);
  if (auto pHeader = baseGraph.GetNodeByName("Header"))
  {
    baseGraph.RemoveNode(pHeader->GetGuid());
  }

  {
    // read the new template as a graph
    ezAbstractObjectGraph leftGraph;
    ezPrefabUtils::LoadGraph(leftGraph, szLeft);
    if (auto pHeader = leftGraph.GetNodeByName("Header"))
    {
      leftGraph.RemoveNode(pHeader->GetGuid());
    }

    // prepare the current state as a graph
    ezAbstractObjectGraph rightGraph;
    {
      ezDocumentObjectConverterWriter writer(&rightGraph, pRight->GetDocumentObjectManager(), true, true);
      writer.AddObjectToGraph(pRight);
      rightGraph.ReMapNodeGuids(PrefabSeed, true);
      // just take the entire ObjectTree node as is TODO: this may cause a crash if the root object is replaced
      rightGraph.CopyNodeIntoGraph(leftGraph.GetNodeByName("ObjectTree"));
    }

    // Merge diffs relative to base
    ezDeque<ezAbstractGraphDiffOperation> mergedDiff;
    ezPrefabUtils::Merge(baseGraph, leftGraph, rightGraph, mergedDiff);


    {
      // Apply merged diff to base.
      baseGraph.ApplyDiff(mergedDiff);

      ezMemoryStreamStorage stor;
      ezMemoryStreamWriter sw(&stor);

#ifdef EZ_ENABLE_DDL
      ezAbstractGraphDdlSerializer::Write(sw, &baseGraph, nullptr, true, ezOpenDdlWriter::TypeStringMode::Shortest);
#else
      ezAbstractGraphJsonSerializer::Write(sw, &baseGraph, nullptr, ezJSONWriter::WhitespaceMode::LessIndentation);
#endif

      out_sNewGraph.SetSubString_ElementCount((const char*)stor.GetData(), stor.GetStorageSize());
    }

    // debug output
    if (PREFAB_DEBUG)
    {
      ezFileWriter file;
      file.Open("C:\\temp\\Prefab - result.txt");
      ezAbstractGraphDdlSerializer::Write(file, &baseGraph, nullptr, false, ezOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }
  }

}

ezString ezPrefabUtils::ReadDocumentAsString(const char* szFile)
{
  ezFileReader file;
  if (file.Open(szFile) == EZ_FAILURE)
  {
    ezLog::Error("Failed to open document file '{0}'", szFile);
    return ezString();
  }

  ezStringBuilder sGraph;
  sGraph.ReadAll(file);

  return sGraph;
}

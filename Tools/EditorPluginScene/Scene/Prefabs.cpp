#include <PCH.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

void ezSceneDocument::UpdatePrefabs()
{
  EZ_LOCK(m_ObjectMetaData.GetMutex());

  // make sure the prefabs are updated
  m_CachedPrefabGraphs.Clear();

  GetCommandHistory()->StartTransaction();

  UpdatePrefabsRecursive(GetObjectManager()->GetRootObject());

  GetCommandHistory()->FinishTransaction();
}

void ezSceneDocument::UpdatePrefabsRecursive(ezDocumentObject* pObject)
{
  auto ChildArray = pObject->GetChildren();

  for (auto pChild : ChildArray)
  {
    auto pMeta = m_ObjectMetaData.BeginReadMetaData(pChild->GetGuid());
    const ezUuid PrefabAsset = pMeta->m_CreateFromPrefab;
    const ezUuid PrefabSeed = pMeta->m_PrefabSeedGuid;
    m_ObjectMetaData.EndReadMetaData();

    // if this is a prefab instance, update it
    if (PrefabAsset.IsValid())
    {
      UpdatePrefabObject(pChild, PrefabAsset, PrefabSeed);
    }
    else
    {
      // only recurse (currently), if no prefab was found
      // that means nested prefabs are currently not possible, that needs to be handled a bit differently later
      UpdatePrefabsRecursive(pChild);
    }
  }
}

void ezSceneDocument::UpdatePrefabObject(ezDocumentObject* pObject, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed)
{
  ezHybridArray<ezUuid, 16> NewObjects;

  ezInstantiatePrefabCommand inst;
  inst.m_bAllowPickedPosition = false;
  inst.m_Parent = pObject->GetParent() == GetObjectManager()->GetRootObject() ? ezUuid() : pObject->GetParent()->GetGuid();
  inst.m_RemapGuid = PrefabSeed;
  inst.m_sJsonGraph = GetCachedPrefabGraph(PrefabAsset);

  NewObjects.Clear();
  void* pArray = &NewObjects;
  memcpy(&inst.m_pCreatedRootObjects, &pArray, sizeof(void*)); /// \todo HACK-o-rama

  /// \todo orientation (in diff?)

  {
    ezAbstractObjectGraph graphOrg;
    ezDocumentObjectConverterWriter writer(&graphOrg, GetObjectManager(), true, true);
    writer.AddObjectToGraph(pObject);

  

  
    ezMemoryStreamStorage storage;
    ezMemoryStreamWriter stringWriter(&storage);
    ezMemoryStreamReader stringReader(&storage);
    stringWriter.WriteBytes(inst.m_sJsonGraph.GetData(), inst.m_sJsonGraph.GetElementCount());

    ezAbstractObjectGraph graphTemplate, graphHeader;
    ezAbstractGraphJsonSerializer::Read(stringReader, &graphTemplate);

    auto pHeader = graphTemplate.GetNodeByName("Header");
    if (pHeader)
    {
      graphHeader.CopyNodeIntoGraph(pHeader);
      graphTemplate.RemoveNode(pHeader->GetGuid());
    }

    graphTemplate.ReMapNodeGuids(PrefabSeed);

    // just take the entire ObjectTree node as is
    graphOrg.CopyNodeIntoGraph(graphTemplate.GetNodeByName("ObjectTree"));


    ezFileWriter file;
    file.Open("D:\\Prefab - org.txt");
    ezAbstractGraphJsonSerializer::Write(file, &graphOrg, ezJSONWriter::WhitespaceMode::LessIndentation);

    ezFileWriter file2;
    file2.Open("D:\\Prefab - template.txt");
    ezAbstractGraphJsonSerializer::Write(file2, &graphTemplate, ezJSONWriter::WhitespaceMode::LessIndentation);

    ezDeque<ezAbstractGraphDiffOperation> DiffResult;
    graphOrg.CreateDiffWithBaseGraph(graphTemplate, DiffResult);

    ezFileWriter file3;
    file3.Open("D:\\Prefab - diff.txt");

    ezStringBuilder sDiff;
    for (const auto& diff : DiffResult)
    {
      switch (diff.m_Operation)
      {
      case ezAbstractGraphDiffOperation::Op::NodeAdd:
        sDiff.AppendFormat("<add> - {%s} (%s)\n", ezConversionUtils::ToString(diff.m_Node).GetData(), diff.m_sProperty.GetData());
        break;

      case ezAbstractGraphDiffOperation::Op::NodeDelete:
        sDiff.AppendFormat("<del> - {%s}\n", ezConversionUtils::ToString(diff.m_Node).GetData());
        break;

      case ezAbstractGraphDiffOperation::Op::PropertySet:
        if (diff.m_Value.CanConvertTo<ezString>())
          sDiff.AppendFormat("<set> - {%s} - \"%s\" = %s\n", ezConversionUtils::ToString(diff.m_Node).GetData(), diff.m_sProperty.GetData(), diff.m_Value.ConvertTo<ezString>().GetData());
        else
          sDiff.AppendFormat("<set> - {%s} - \"%s\" = xxx\n", ezConversionUtils::ToString(diff.m_Node).GetData(), diff.m_sProperty.GetData());
        break;

      }
    }

    /// \todo
    // copied ObjectTree references are wrong
    // uuid remapping must be undone

    file3.WriteBytes(sDiff.GetData(), sDiff.GetElementCount());

    {
      graphTemplate.ApplyDiff(DiffResult);
      graphTemplate.ReMapNodeGuids(PrefabSeed, true);

      graphTemplate.CopyNodeIntoGraph(graphHeader.GetNodeByName("Header"));

      ezMemoryStreamStorage stor;
      ezMemoryStreamWriter sw(&stor);

      ezAbstractGraphJsonSerializer::Write(sw, &graphTemplate, ezJSONWriter::WhitespaceMode::LessIndentation);

      ezStringBuilder sNewGraph;
      sNewGraph.SetSubString_ElementCount((const char*)stor.GetData(), stor.GetStorageSize());

      inst.m_sJsonGraph = sNewGraph;

      ezFileWriter file4;
      file4.Open("D:\\Prefab - result.txt");
      file4.WriteBytes(inst.m_sJsonGraph.GetData(), inst.m_sJsonGraph.GetElementCount());

    }
  }

  ezRemoveObjectCommand rm;
  rm.m_Object = pObject->GetGuid();

  // remove current object
  GetCommandHistory()->AddCommand(rm);

  // instantiate prefab again
  GetCommandHistory()->AddCommand(inst);

  // pass the prefab meta data to the new instance
  for (const auto& guid : NewObjects)
  {
    auto pMeta = m_ObjectMetaData.BeginModifyMetaData(guid);
    pMeta->m_CreateFromPrefab = PrefabAsset;
    pMeta->m_PrefabSeedGuid = PrefabSeed;

    m_ObjectMetaData.EndModifyMetaData(ezSceneObjectMetaData::PrefabFlag);
  }
}
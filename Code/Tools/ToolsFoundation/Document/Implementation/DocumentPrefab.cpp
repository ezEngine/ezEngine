#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

#include <Foundation/IO/MemoryStream.h>

ezString ToBinary(const ezUuid& guid)
{
  ezStringBuilder s, sResult;

  ezUInt8* pBytes = (ezUInt8*)&guid;

  for (ezUInt32 i = 0; i < sizeof(ezUuid); ++i)
  {
    s.Format("%02X", (ezUInt32)*pBytes);
    ++pBytes;

    sResult.Append(s);
  }

  return sResult;
}

void WriteDiff(ezDeque<ezAbstractGraphDiffOperation> mergedDiff, ezStringBuilder &sDiff)
{
  for (const auto& diff : mergedDiff)
  {
    ezStringBuilder Data = ToBinary(diff.m_Node);

    switch (diff.m_Operation)
    {
    case ezAbstractGraphDiffOperation::Op::NodeAdded:
      {
        sDiff.AppendFormat("<add> - {%s} (%s)\n", Data.GetData(), diff.m_sProperty.GetData());
      }
      break;

    case ezAbstractGraphDiffOperation::Op::NodeRemoved:
      {
        sDiff.AppendFormat("<del> - {%s}\n", Data.GetData());
      }
      break;

    case ezAbstractGraphDiffOperation::Op::PropertyChanged:
      if (diff.m_Value.CanConvertTo<ezString>())
        sDiff.AppendFormat("<set> - {%s} - \"%s\" = %s\n", Data.GetData(), diff.m_sProperty.GetData(), diff.m_Value.ConvertTo<ezString>().GetData());
      else
        sDiff.AppendFormat("<set> - {%s} - \"%s\" = xxx\n", Data.GetData(), diff.m_sProperty.GetData());
      break;

    }
  }
}

void ezDocument::UpdatePrefabs()
{
  // make sure the prefabs are updated
  m_CachedPrefabGraphs.Clear();

  GetCommandHistory()->StartTransaction();

  UpdatePrefabsRecursive(GetObjectManager()->GetRootObject());

  GetCommandHistory()->FinishTransaction();

  ShowDocumentStatus("Prefabs have been updated");
}

void ezDocument::RevertPrefabs(const ezDeque<const ezDocumentObject*>& Selection)
{
  if (Selection.IsEmpty())
    return;

  auto pHistory = GetCommandHistory();

  m_CachedPrefabGraphs.Clear();

  pHistory->StartTransaction();

  for (auto pItem : Selection)
  {
    RevertPrefab(pItem);
  }

  pHistory->FinishTransaction();
}

void ezDocument::UnlinkPrefabs(const ezDeque<const ezDocumentObject*>& Selection)
{
  if (Selection.IsEmpty())
    return;

  /// \todo this operation is (currently) not undo-able, since it only operates on meta data

  for (auto pObject : Selection)
  {
    auto pMetaDoc = m_DocumentObjectMetaData.BeginModifyMetaData(pObject->GetGuid());
    pMetaDoc->m_CreateFromPrefab = ezUuid();
    pMetaDoc->m_PrefabSeedGuid = ezUuid();
    pMetaDoc->m_sBasePrefab.Clear();
    m_DocumentObjectMetaData.EndModifyMetaData(ezDocumentObjectMetaData::PrefabFlag);
  }
}

const ezString& ezDocument::GetCachedPrefabGraph(const ezUuid& documentGuid)
{
  if (!m_CachedPrefabGraphs.Contains(documentGuid))
  {
    ezString sPrefabFile = GetDocumentPathFromGuid(documentGuid);

    m_CachedPrefabGraphs[documentGuid] = ReadDocumentAsString(sPrefabFile);
  }

  return m_CachedPrefabGraphs[documentGuid];
}

ezStatus ezDocument::CreatePrefabDocumentFromSelection(const char* szFile, const ezRTTI* pRootType)
{
  auto Selection = GetSelectionManager()->GetTopLevelSelection(pRootType);

  if (Selection.GetCount() != 1)
    return ezStatus("To create a prefab, the selection must contain exactly one game object");

  const ezDocumentObject* pNode = Selection[0];

  ezUuid PrefabGuid, SeedGuid;
  SeedGuid.CreateNewUuid();
  ezStatus res = CreatePrefabDocument(szFile, pNode, SeedGuid, PrefabGuid);

  if (res.m_Result.Succeeded())
  {
    ReplaceByPrefab(pNode, szFile, PrefabGuid, SeedGuid);
  }

  return res;
}

ezStatus ezDocument::CreatePrefabDocument(const char* szFile, const ezDocumentObject* pSaveAsPrefab, const ezUuid& invPrefabSeed, ezUuid& out_NewDocumentGuid)
{
  EZ_ASSERT_DEV(pSaveAsPrefab != nullptr, "CreatePrefabDocument: pSaveAsPrefab must be a valod object!");
  const ezRTTI* pRootType = pSaveAsPrefab->GetTypeAccessor().GetType();
  ezDocumentManager* pDocumentManager;
  if (ezDocumentManager::FindDocumentTypeFromPath(szFile, true, pDocumentManager).Failed())
    return ezStatus("Document type is unknown: '%s'", szFile);

  if (pDocumentManager->EnsureDocumentIsClosed(szFile).Failed())
  {
    return ezStatus("Could not close the existing prefab document");
  }

  // prepare the current state as a graph
  ezAbstractObjectGraph PrefabGraph;

  ezDocumentObjectConverterWriter writer(&PrefabGraph, GetObjectManager(), true, true);
  auto pPrefabGraphMainNode = writer.AddObjectToGraph(pSaveAsPrefab);

  PrefabGraph.ReMapNodeGuids(invPrefabSeed, true);

  ezDocument* pSceneDocument = nullptr;

  {
    auto res = pDocumentManager->CreateDocument("ezPrefab", szFile, pSceneDocument, false);

    if (res.m_Result.Failed())
      return res;
  }

  out_NewDocumentGuid = pSceneDocument->GetGuid();

  ezUuid rootGuid = pSaveAsPrefab->GetGuid();
  rootGuid.RevertCombinationWithSeed(invPrefabSeed);

  auto pPrefabSceneRoot = pSceneDocument->GetObjectManager()->GetRootObject();
  ezDocumentObject* pPrefabSceneMainObject = pSceneDocument->GetObjectManager()->CreateObject(pRootType, rootGuid);
  pSceneDocument->GetObjectManager()->AddObject(pPrefabSceneMainObject, pPrefabSceneRoot, "Children", 0);

  ezDocumentObjectConverterReader reader(&PrefabGraph, pSceneDocument->GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  reader.ApplyPropertiesToObject(pPrefabGraphMainNode, pPrefabSceneMainObject);

  auto res = pSceneDocument->SaveDocument();
  pDocumentManager->CloseDocument(pSceneDocument);
  return res;
}


ezUuid ezDocument::ReplaceByPrefab(const ezDocumentObject* pRootObject, const char* szPrefabFile, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed)
{
  GetCommandHistory()->StartTransaction();

  ezRemoveObjectCommand remCmd;
  remCmd.m_Object = pRootObject->GetGuid();

  ezInstantiatePrefabCommand instCmd;
  instCmd.m_bAllowPickedPosition = false;
  instCmd.m_Parent = pRootObject->GetParent() == GetObjectManager()->GetRootObject() ? ezUuid() : pRootObject->GetParent()->GetGuid();
  instCmd.m_sJsonGraph = ReadDocumentAsString(szPrefabFile); // since the prefab might have been created just now, going through the cache (via GUID) will most likely fail
  instCmd.m_RemapGuid = PrefabSeed;

  GetCommandHistory()->AddCommand(remCmd);
  GetCommandHistory()->AddCommand(instCmd);
  GetCommandHistory()->FinishTransaction();

  if (instCmd.m_CreatedRootObject.IsValid())
  {
    auto pMeta = m_DocumentObjectMetaData.BeginModifyMetaData(instCmd.m_CreatedRootObject);
    pMeta->m_CreateFromPrefab = PrefabAsset;
    pMeta->m_PrefabSeedGuid = PrefabSeed;
    pMeta->m_sBasePrefab = instCmd.m_sJsonGraph;
    m_DocumentObjectMetaData.EndModifyMetaData(ezDocumentObjectMetaData::PrefabFlag);
  }
  return instCmd.m_CreatedRootObject;
}

ezUuid ezDocument::RevertPrefab(const ezDocumentObject* pObject)
{
  auto pHistory = GetCommandHistory();
  auto pMeta = m_DocumentObjectMetaData.BeginReadMetaData(pObject->GetGuid());

  const ezUuid PrefabAsset = pMeta->m_CreateFromPrefab;

  if (!PrefabAsset.IsValid())
  {
    m_DocumentObjectMetaData.EndReadMetaData();
    return ezUuid();
  }

  ezRemoveObjectCommand remCmd;
  remCmd.m_Object = pObject->GetGuid();

  ezInstantiatePrefabCommand instCmd;
  instCmd.m_bAllowPickedPosition = false;
  instCmd.m_Parent = pObject->GetParent() == GetObjectManager()->GetRootObject() ? ezUuid() : pObject->GetParent()->GetGuid();
  instCmd.m_RemapGuid = pMeta->m_PrefabSeedGuid;
  instCmd.m_sJsonGraph = GetCachedPrefabGraph(pMeta->m_CreateFromPrefab);

  m_DocumentObjectMetaData.EndReadMetaData();

  pHistory->AddCommand(remCmd);
  pHistory->AddCommand(instCmd);

  if (instCmd.m_CreatedRootObject.IsValid())
  {
    auto pMeta = m_DocumentObjectMetaData.BeginModifyMetaData(instCmd.m_CreatedRootObject);
    pMeta->m_CreateFromPrefab = PrefabAsset;
    pMeta->m_PrefabSeedGuid = instCmd.m_RemapGuid;
    pMeta->m_sBasePrefab = instCmd.m_sJsonGraph;

    m_DocumentObjectMetaData.EndModifyMetaData(ezDocumentObjectMetaData::PrefabFlag);
  }
  return instCmd.m_CreatedRootObject;
}


void ezDocument::UpdatePrefabsRecursive(ezDocumentObject* pObject)
{
  auto ChildArray = pObject->GetChildren();

  ezStringBuilder sPrefabBase;

  for (auto pChild : ChildArray)
  {
    auto pMeta = m_DocumentObjectMetaData.BeginReadMetaData(pChild->GetGuid());
    const ezUuid PrefabAsset = pMeta->m_CreateFromPrefab;
    const ezUuid PrefabSeed = pMeta->m_PrefabSeedGuid;
    sPrefabBase = pMeta->m_sBasePrefab;

    m_DocumentObjectMetaData.EndReadMetaData();

    // if this is a prefab instance, update it
    if (PrefabAsset.IsValid())
    {
      UpdatePrefabObject(pChild, PrefabAsset, PrefabSeed, sPrefabBase);
    }
    else
    {
      // only recurse (currently), if no prefab was found
      // that means nested prefabs are currently not possible, that needs to be handled a bit differently later
      UpdatePrefabsRecursive(pChild);
    }
  }
}

void ezDocument::UpdatePrefabObject(ezDocumentObject* pObject, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed, const char* szBasePrefab)
{
  ezInstantiatePrefabCommand inst;
  inst.m_bAllowPickedPosition = false;
  inst.m_Parent = pObject->GetParent() == GetObjectManager()->GetRootObject() ? ezUuid() : pObject->GetParent()->GetGuid();
  inst.m_RemapGuid = PrefabSeed;
  inst.m_sJsonGraph = GetCachedPrefabGraph(PrefabAsset);

  // prepare the original prefab as a graph
  ezAbstractObjectGraph graphBasePrefab;
  {
    ezMemoryStreamStorage storage;
    ezMemoryStreamWriter stringWriter(&storage);
    ezMemoryStreamReader stringReader(&storage);
    stringWriter.WriteBytes(szBasePrefab, ezStringUtils::GetStringElementCount(szBasePrefab));

    ezAbstractGraphJsonSerializer::Read(stringReader, &graphBasePrefab);
  }

  {
    ezAbstractObjectGraph graphNewPrefab, graphHeader;

    // read the new template as a graph
    {
      ezMemoryStreamStorage storage;
      ezMemoryStreamWriter stringWriter(&storage);
      ezMemoryStreamReader stringReader(&storage);
      stringWriter.WriteBytes(inst.m_sJsonGraph.GetData(), inst.m_sJsonGraph.GetElementCount());
      ezAbstractGraphJsonSerializer::Read(stringReader, &graphNewPrefab);
    }

    // remove the header
    {
      auto pHeader = graphNewPrefab.GetNodeByName("Header");
      EZ_ASSERT_DEBUG(pHeader, "header is missing");
      graphNewPrefab.RemoveNode(pHeader->GetGuid());
    }

    // prepare the current state as a graph
    ezAbstractObjectGraph graphCurrentInstance;
    {
      ezDocumentObjectConverterWriter writer(&graphCurrentInstance, GetObjectManager(), true, true);
      writer.AddObjectToGraph(pObject);
      graphCurrentInstance.ReMapNodeGuids(PrefabSeed, true);
      // just take the entire ObjectTree node as is
      graphCurrentInstance.CopyNodeIntoGraph(graphNewPrefab.GetNodeByName("ObjectTree"));
    }


    // debug output
    if (false)
    {
      {
        ezFileWriter file;
        file.Open("D:\\Prefab - base.txt");
        file.WriteBytes(szBasePrefab, ezStringUtils::GetStringElementCount(szBasePrefab));
      }

      {
        ezFileWriter file;
        file.Open("D:\\Prefab - org.txt");
        ezAbstractGraphJsonSerializer::Write(file, &graphCurrentInstance, nullptr, ezJSONWriter::WhitespaceMode::LessIndentation);
      }

      {
        ezFileWriter file2;
        file2.Open("D:\\Prefab - template.txt");
        ezAbstractGraphJsonSerializer::Write(file2, &graphNewPrefab, nullptr, ezJSONWriter::WhitespaceMode::LessIndentation);
      }
    }


    ezDeque<ezAbstractGraphDiffOperation> InstanceToBase;
    graphCurrentInstance.CreateDiffWithBaseGraph(graphBasePrefab, InstanceToBase);
    ezDeque<ezAbstractGraphDiffOperation> TemplateToBase;
    graphNewPrefab.CreateDiffWithBaseGraph(graphBasePrefab, TemplateToBase);
    ezDeque<ezAbstractGraphDiffOperation> mergedDiff;
    graphBasePrefab.MergeDiffs(TemplateToBase, InstanceToBase, mergedDiff);

    // debug output
    if (false)
    {
      ezFileWriter file3;
      file3.Open("D:\\Prefab - diff.txt");

      ezStringBuilder sDiff;
      sDiff.Append("######## Template To Base #######\n");
      WriteDiff(TemplateToBase, sDiff);
      sDiff.Append("\n\n######## Instance To Base #######\n");
      WriteDiff(InstanceToBase, sDiff);
      sDiff.Append("\n\n######## Merged Diff #######\n");
      WriteDiff(mergedDiff, sDiff);


      file3.WriteBytes(sDiff.GetData(), sDiff.GetElementCount());
    }

    {
      //graphBasePrefab.CopyNodeIntoGraph(graphHeader.GetNodeByName("Header"));
      graphBasePrefab.ApplyDiff(mergedDiff);

      ezMemoryStreamStorage stor;
      ezMemoryStreamWriter sw(&stor);

      ezAbstractGraphJsonSerializer::Write(sw, &graphBasePrefab, nullptr, ezJSONWriter::WhitespaceMode::LessIndentation);

      ezStringBuilder sNewGraph;
      sNewGraph.SetSubString_ElementCount((const char*)stor.GetData(), stor.GetStorageSize());

      inst.m_sJsonGraph = sNewGraph;
    }

    // debug output
    if (false)
    {
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
  if (inst.m_CreatedRootObject.IsValid())
  {
    auto pMeta = m_DocumentObjectMetaData.BeginModifyMetaData(inst.m_CreatedRootObject);
    pMeta->m_CreateFromPrefab = PrefabAsset;
    pMeta->m_PrefabSeedGuid = PrefabSeed;
    pMeta->m_sBasePrefab = GetCachedPrefabGraph(PrefabAsset);

    m_DocumentObjectMetaData.EndModifyMetaData(ezDocumentObjectMetaData::PrefabFlag);
  }
}

ezString ezDocument::ReadDocumentAsString(const char* szFile) const
{
  ezFileReader file;
  if (file.Open(szFile) == EZ_FAILURE)
  {
    ezLog::Error("Failed to open document file '%s'", szFile);
    return ezString();
  }

  ezStringBuilder sGraph;
  sGraph.ReadAll(file);

  return sGraph;
}

ezString ezDocument::GetDocumentPathFromGuid(const ezUuid& documentGuid) const
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezString();
}

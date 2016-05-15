#include <PCH.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Core/World/GameObject.h>

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

ezStatus ezSceneDocument::CreatePrefabDocumentFromSelection(const char* szFile)
{
  auto Selection = GetSelectionManager()->GetTopLevelSelection(ezGetStaticRTTI<ezGameObject>());

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

ezStatus ezSceneDocument::CreatePrefabDocument(const char* szFile, const ezDocumentObject* pSaveAsPrefab, const ezUuid& invPrefabSeed, ezUuid& out_NewDocumentGuid)
{
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
  ezDocumentObject* pPrefabSceneMainObject = pSceneDocument->GetObjectManager()->CreateObject(ezGetStaticRTTI<ezGameObject>(), rootGuid);
  pSceneDocument->GetObjectManager()->AddObject(pPrefabSceneMainObject, pPrefabSceneRoot, "Children", 0);

  ezDocumentObjectConverterReader reader(&PrefabGraph, pSceneDocument->GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  reader.ApplyPropertiesToObject(pPrefabGraphMainNode, pPrefabSceneMainObject);

  auto res = pSceneDocument->SaveDocument();
  pDocumentManager->CloseDocument(pSceneDocument);
  return res;
}


void ezSceneDocument::ReplaceByPrefab(const ezDocumentObject* pRootObject, const char* szPrefabFile, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed)
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
    {
      auto pMeta = m_DocumentObjectMetaData.BeginModifyMetaData(instCmd.m_CreatedRootObject);
      pMeta->m_CreateFromPrefab = PrefabAsset;
      pMeta->m_PrefabSeedGuid = PrefabSeed;
      pMeta->m_sBasePrefab = instCmd.m_sJsonGraph;
      m_DocumentObjectMetaData.EndModifyMetaData(ezDocumentObjectMetaData::PrefabFlag);
    }

    {
      auto pMeta = m_SceneObjectMetaData.BeginModifyMetaData(instCmd.m_CreatedRootObject);
      pMeta->m_CachedNodeName.Clear();
      m_SceneObjectMetaData.EndModifyMetaData(ezSceneObjectMetaData::CachedName);
    }
  }
}

void ezSceneDocument::UpdatePrefabs()
{
  EZ_LOCK(m_SceneObjectMetaData.GetMutex());

  // make sure the prefabs are updated
  m_CachedPrefabGraphs.Clear();

  GetCommandHistory()->StartTransaction();

  UpdatePrefabsRecursive(GetObjectManager()->GetRootObject());

  GetCommandHistory()->FinishTransaction();

  ShowDocumentStatus("Prefabs have been updated");
}

void ezSceneDocument::UpdatePrefabsRecursive(ezDocumentObject* pObject)
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

void ezSceneDocument::UpdatePrefabObject(ezDocumentObject* pObject, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed, const char* szBasePrefab)
{
  ezUuid NewObject;

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

    // copy the header, to be able to insert it into the result later
    auto pHeader = graphNewPrefab.GetNodeByName("Header");
    {
      EZ_ASSERT_DEBUG(pHeader, "header is missing");
      graphHeader.CopyNodeIntoGraph(pHeader);
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

    ezDeque<ezAbstractGraphDiffOperation> DiffToBase;
    graphCurrentInstance.CreateDiffWithBaseGraph(graphBasePrefab, DiffToBase);

    // debug output
    if (false)
    {
      ezFileWriter file3;
      file3.Open("D:\\Prefab - diff.txt");

      ezStringBuilder sDiff;
      for (const auto& diff : DiffToBase)
      {
        ezStringBuilder Data = ToBinary(diff.m_Node);

        switch (diff.m_Operation)
        {
        case ezAbstractGraphDiffOperation::Op::NodeAdd:
          {
            sDiff.AppendFormat("<add> - {%s} (%s)\n", Data.GetData(), diff.m_sProperty.GetData());
          }
          break;

        case ezAbstractGraphDiffOperation::Op::NodeDelete:
          {
            sDiff.AppendFormat("<del> - {%s}\n", Data.GetData());
          }
          break;

        case ezAbstractGraphDiffOperation::Op::PropertySet:
          if (diff.m_Value.CanConvertTo<ezString>())
            sDiff.AppendFormat("<set> - {%s} - \"%s\" = %s\n", Data.GetData(), diff.m_sProperty.GetData(), diff.m_Value.ConvertTo<ezString>().GetData());
          else
            sDiff.AppendFormat("<set> - {%s} - \"%s\" = xxx\n", Data.GetData(), diff.m_sProperty.GetData());
          break;

        }
      }

      file3.WriteBytes(sDiff.GetData(), sDiff.GetElementCount());
    }

    {
      graphNewPrefab.CopyNodeIntoGraph(graphHeader.GetNodeByName("Header"));
      graphNewPrefab.ApplyDiff(DiffToBase);

      ezMemoryStreamStorage stor;
      ezMemoryStreamWriter sw(&stor);

      ezAbstractGraphJsonSerializer::Write(sw, &graphNewPrefab, nullptr, ezJSONWriter::WhitespaceMode::LessIndentation);

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
    auto pMeta = m_DocumentObjectMetaData.BeginModifyMetaData(NewObject);
    pMeta->m_CreateFromPrefab = PrefabAsset;
    pMeta->m_PrefabSeedGuid = PrefabSeed;
    pMeta->m_sBasePrefab = GetCachedPrefabGraph(PrefabAsset);

    m_DocumentObjectMetaData.EndModifyMetaData(ezDocumentObjectMetaData::PrefabFlag);
  }
}
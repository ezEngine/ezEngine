#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

void ezDocument::UpdatePrefabs()
{
  GetCommandHistory()->StartTransaction("Update Prefabs");

  UpdatePrefabsRecursive(GetObjectManager()->GetRootObject());

  GetCommandHistory()->FinishTransaction();

  ShowDocumentStatus("Prefabs have been updated");
  SetModified(true);
}

void ezDocument::RevertPrefabs(ezArrayPtr<const ezDocumentObject*> selection)
{
  if (selection.IsEmpty())
    return;

  auto pHistory = GetCommandHistory();

  pHistory->StartTransaction("Revert Prefab");

  for (auto pItem : selection)
  {
    RevertPrefab(pItem);
  }

  pHistory->FinishTransaction();
}

void ezDocument::UnlinkPrefabs(ezArrayPtr<const ezDocumentObject*> selection)
{
  if (selection.IsEmpty())
    return;

  auto pHistory = GetCommandHistory();
  pHistory->StartTransaction("Unlink Prefab");

  for (auto pObject : selection)
  {
    ezUnlinkPrefabCommand cmd;
    cmd.m_Object = pObject->GetGuid();

    pHistory->AddCommand(cmd).AssertSuccess();
  }

  pHistory->FinishTransaction();
}

ezStatus ezDocument::CreatePrefabDocumentFromSelection(ezStringView sFile, const ezRTTI* pRootType, ezDelegate<void(ezAbstractObjectNode*)> adjustGraphNodeCB, ezDelegate<void(ezDocumentObject*)> adjustNewNodesCB, ezDelegate<void(ezAbstractObjectGraph& graph, ezDynamicArray<ezAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB)
{
  ezHybridArray<ezSelectionEntry, 64> selection;
  GetSelectionManager()->GetTopLevelSelectionOfType(pRootType, selection);

  if (selection.IsEmpty())
    return ezStatus("To create a prefab, the selection must not be empty");

  ezHybridArray<const ezDocumentObject*, 32> nodes;
  nodes.Reserve(selection.GetCount());
  for (const auto& e : selection)
  {
    nodes.PushBack(e.m_pObject);
  }

  ezUuid PrefabGuid, SeedGuid;
  SeedGuid = ezUuid::MakeUuid();
  ezStatus res = CreatePrefabDocument(sFile, nodes, SeedGuid, PrefabGuid, adjustGraphNodeCB, true, finalizeGraphCB);

  if (res.Succeeded())
  {
    GetCommandHistory()->StartTransaction("Replace all by Prefab");

    // this replaces ONE object by the new prefab (we pick the last one in the selection)
    ezUuid newObj = ReplaceByPrefab(nodes.PeekBack(), sFile, PrefabGuid, SeedGuid, true);

    // if we had more than one selected objects, remove the others as well
    if (nodes.GetCount() > 1)
    {
      nodes.PopBack();

      for (auto pNode : nodes)
      {
        ezRemoveObjectCommand remCmd;
        remCmd.m_Object = pNode->GetGuid();

        GetCommandHistory()->AddCommand(remCmd).AssertSuccess();
      }
    }

    auto pObject = GetObjectManager()->GetObject(newObj);

    if (adjustNewNodesCB.IsValid())
    {
      adjustNewNodesCB(pObject);
    }

    GetCommandHistory()->FinishTransaction();
    GetSelectionManager()->SetSelection(pObject);
  }

  return res;
}

ezStatus ezDocument::CreatePrefabDocument(ezStringView sFile, ezArrayPtr<const ezDocumentObject*> rootObjects, const ezUuid& invPrefabSeed,
  ezUuid& out_newDocumentGuid, ezDelegate<void(ezAbstractObjectNode*)> adjustGraphNodeCB, bool bKeepOpen, ezDelegate<void(ezAbstractObjectGraph& graph, ezDynamicArray<ezAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB)
{
  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(sFile, true, pTypeDesc).Failed())
    return ezStatus(ezFmt("Document type is unknown: '{0}'", sFile));

  pTypeDesc->m_pManager->EnsureDocumentIsClosed(sFile);

  // prepare the current state as a graph
  ezAbstractObjectGraph PrefabGraph;
  ezDocumentObjectConverterWriter writer(&PrefabGraph, GetObjectManager());

  ezHybridArray<ezAbstractObjectNode*, 32> graphRootNodes;
  graphRootNodes.Reserve(rootObjects.GetCount() + 1);

  for (ezUInt32 i = 0; i < rootObjects.GetCount(); ++i)
  {
    auto pSaveAsPrefab = rootObjects[i];

    EZ_ASSERT_DEV(pSaveAsPrefab != nullptr, "CreatePrefabDocument: pSaveAsPrefab must be a valid object!");

    auto pPrefabGraphMainNode = writer.AddObjectToGraph(pSaveAsPrefab);
    graphRootNodes.PushBack(pPrefabGraphMainNode);

    // allow external adjustments
    if (adjustGraphNodeCB.IsValid())
    {
      adjustGraphNodeCB(pPrefabGraphMainNode);
    }
  }

  if (finalizeGraphCB.IsValid())
  {
    finalizeGraphCB(PrefabGraph, graphRootNodes);
  }

  PrefabGraph.ReMapNodeGuids(invPrefabSeed, true);

  ezDocument* pSceneDocument = nullptr;

  EZ_SUCCEED_OR_RETURN(pTypeDesc->m_pManager->CreateDocument("Prefab", sFile, pSceneDocument, ezDocumentFlags::RequestWindow | ezDocumentFlags::AddToRecentFilesList | ezDocumentFlags::EmptyDocument));

  out_newDocumentGuid = pSceneDocument->GetGuid();
  auto pPrefabSceneRoot = pSceneDocument->GetObjectManager()->GetRootObject();

  ezDocumentObjectConverterReader reader(&PrefabGraph, pSceneDocument->GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument);

  for (ezUInt32 i = 0; i < graphRootNodes.GetCount(); ++i)
  {
    const ezRTTI* pRootType = ezRTTI::FindTypeByName(graphRootNodes[i]->GetType());

    ezUuid rootGuid = graphRootNodes[i]->GetGuid();
    rootGuid.RevertCombinationWithSeed(invPrefabSeed);

    ezDocumentObject* pPrefabSceneMainObject = pSceneDocument->GetObjectManager()->CreateObject(pRootType, rootGuid);
    pSceneDocument->GetObjectManager()->AddObject(pPrefabSceneMainObject, pPrefabSceneRoot, "Children", -1);

    reader.ApplyPropertiesToObject(graphRootNodes[i], pPrefabSceneMainObject);
  }

  pSceneDocument->SetModified(true);
  auto res = pSceneDocument->SaveDocument();

  if (!bKeepOpen)
  {
    pTypeDesc->m_pManager->CloseDocument(pSceneDocument);
  }

  return res;
}


ezUuid ezDocument::ReplaceByPrefab(const ezDocumentObject* pRootObject, ezStringView sPrefabFile, const ezUuid& prefabAsset, const ezUuid& prefabSeed, bool bEnginePrefab)
{
  GetCommandHistory()->StartTransaction("Replace by Prefab");

  ezUuid instantiatedRoot;

  if (!bEnginePrefab) // create editor prefab
  {
    ezInstantiatePrefabCommand instCmd;
    instCmd.m_Index = pRootObject->GetPropertyIndex().ConvertTo<ezInt32>();
    instCmd.m_bAllowPickedPosition = false;
    instCmd.m_CreateFromPrefab = prefabAsset;
    instCmd.m_Parent = pRootObject->GetParent() == GetObjectManager()->GetRootObject() ? ezUuid() : pRootObject->GetParent()->GetGuid();
    instCmd.m_sBasePrefabGraph = ezPrefabUtils::ReadDocumentAsString(
      sPrefabFile); // since the prefab might have been created just now, going through the cache (via GUID) will most likely fail
    instCmd.m_RemapGuid = prefabSeed;

    GetCommandHistory()->AddCommand(instCmd).AssertSuccess();

    instantiatedRoot = instCmd.m_CreatedRootObject;
  }
  else // create an object with the reference prefab component
  {
    auto pHistory = GetCommandHistory();

    ezStringBuilder tmp;
    ezUuid CmpGuid = ezUuid::MakeUuid();
    instantiatedRoot = ezUuid::MakeUuid();

    ezAddObjectCommand cmd;
    cmd.m_Parent = (pRootObject->GetParent() == GetObjectManager()->GetRootObject()) ? ezUuid() : pRootObject->GetParent()->GetGuid();
    cmd.m_Index = pRootObject->GetPropertyIndex();
    cmd.SetType("ezGameObject");
    cmd.m_NewObjectGuid = instantiatedRoot;
    cmd.m_sParentProperty = "Children";

    EZ_VERIFY(pHistory->AddCommand(cmd).Succeeded(), "AddCommand failed");

    cmd.SetType("ezPrefabReferenceComponent");
    cmd.m_sParentProperty = "Components";
    cmd.m_Index = -1;
    cmd.m_NewObjectGuid = CmpGuid;
    cmd.m_Parent = instantiatedRoot;
    EZ_VERIFY(pHistory->AddCommand(cmd).Succeeded(), "AddCommand failed");

    ezSetObjectPropertyCommand cmd2;
    cmd2.m_Object = CmpGuid;
    cmd2.m_sProperty = "Prefab";
    cmd2.m_NewValue = ezConversionUtils::ToString(prefabAsset, tmp).GetData();
    EZ_VERIFY(pHistory->AddCommand(cmd2).Succeeded(), "AddCommand failed");
  }

  {
    ezRemoveObjectCommand remCmd;
    remCmd.m_Object = pRootObject->GetGuid();

    GetCommandHistory()->AddCommand(remCmd).AssertSuccess();
  }

  GetCommandHistory()->FinishTransaction();

  return instantiatedRoot;
}

ezUuid ezDocument::RevertPrefab(const ezDocumentObject* pObject)
{
  auto pHistory = GetCommandHistory();
  auto pMeta = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());

  const ezUuid PrefabAsset = pMeta->m_CreateFromPrefab;

  if (!PrefabAsset.IsValid())
  {
    m_DocumentObjectMetaData->EndReadMetaData();
    return ezUuid();
  }

  ezRemoveObjectCommand remCmd;
  remCmd.m_Object = pObject->GetGuid();

  ezInstantiatePrefabCommand instCmd;
  instCmd.m_Index = pObject->GetPropertyIndex().ConvertTo<ezInt32>();
  instCmd.m_bAllowPickedPosition = false;
  instCmd.m_CreateFromPrefab = PrefabAsset;
  instCmd.m_Parent = pObject->GetParent() == GetObjectManager()->GetRootObject() ? ezUuid() : pObject->GetParent()->GetGuid();
  instCmd.m_RemapGuid = pMeta->m_PrefabSeedGuid;
  instCmd.m_sBasePrefabGraph = ezPrefabCache::GetSingleton()->GetCachedPrefabDocument(pMeta->m_CreateFromPrefab);

  m_DocumentObjectMetaData->EndReadMetaData();

  pHistory->AddCommand(remCmd).AssertSuccess();
  pHistory->AddCommand(instCmd).AssertSuccess();

  return instCmd.m_CreatedRootObject;
}


void ezDocument::UpdatePrefabsRecursive(ezDocumentObject* pObject)
{
  // Deliberately copy the array as the UpdatePrefabObject function will add / remove elements from the array.
  auto ChildArray = pObject->GetChildren();

  ezStringBuilder sPrefabBase;

  for (auto pChild : ChildArray)
  {
    auto pMeta = m_DocumentObjectMetaData->BeginReadMetaData(pChild->GetGuid());
    const ezUuid PrefabAsset = pMeta->m_CreateFromPrefab;
    const ezUuid PrefabSeed = pMeta->m_PrefabSeedGuid;
    sPrefabBase = pMeta->m_sBasePrefab;

    m_DocumentObjectMetaData->EndReadMetaData();

    // if this is a prefab instance, update it
    if (PrefabAsset.IsValid())
    {
      UpdatePrefabObject(pChild, PrefabAsset, PrefabSeed, sPrefabBase);
    }
    else
    {
      // only recurse if no prefab was found
      // nested prefabs are not allowed
      UpdatePrefabsRecursive(pChild);
    }
  }
}

void ezDocument::UpdatePrefabObject(ezDocumentObject* pObject, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed, ezStringView sBasePrefab)
{
  const ezStringBuilder& sNewBasePrefab = ezPrefabCache::GetSingleton()->GetCachedPrefabDocument(PrefabAsset);

  ezStringBuilder sNewMergedGraph;
  ezPrefabUtils::Merge(sBasePrefab, sNewBasePrefab, pObject, true, PrefabSeed, sNewMergedGraph);

  // remove current object
  ezRemoveObjectCommand rm;
  rm.m_Object = pObject->GetGuid();

  // instantiate prefab again
  ezInstantiatePrefabCommand inst;
  inst.m_Index = pObject->GetPropertyIndex().ConvertTo<ezInt32>();
  inst.m_bAllowPickedPosition = false;
  inst.m_CreateFromPrefab = PrefabAsset;
  inst.m_Parent = pObject->GetParent() == GetObjectManager()->GetRootObject() ? ezUuid() : pObject->GetParent()->GetGuid();
  inst.m_RemapGuid = PrefabSeed;
  inst.m_sBasePrefabGraph = sNewBasePrefab;
  inst.m_sObjectGraph = sNewMergedGraph;

  GetCommandHistory()->AddCommand(rm).AssertSuccess();
  GetCommandHistory()->AddCommand(inst).AssertSuccess();
}

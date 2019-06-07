#include <ToolsFoundationPCH.h>

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

void ezDocument::RevertPrefabs(const ezDeque<const ezDocumentObject*>& Selection)
{
  if (Selection.IsEmpty())
    return;

  auto pHistory = GetCommandHistory();

  pHistory->StartTransaction("Revert Prefab");

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

  auto pHistory = GetCommandHistory();
  pHistory->StartTransaction("Unlink Prefab");

  for (auto pObject : Selection)
  {
    ezUnlinkPrefabCommand cmd;
    cmd.m_Object = pObject->GetGuid();

    pHistory->AddCommand(cmd);
  }

  pHistory->FinishTransaction();
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
    ezUuid newObj = ReplaceByPrefab(pNode, szFile, PrefabGuid, SeedGuid);

    auto pObject = GetObjectManager()->GetObject(newObj);
    GetSelectionManager()->SetSelection(pObject);
  }

  return res;
}

ezStatus ezDocument::CreatePrefabDocument(
  const char* szFile, const ezDocumentObject* pSaveAsPrefab, const ezUuid& invPrefabSeed, ezUuid& out_NewDocumentGuid)
{
  EZ_ASSERT_DEV(pSaveAsPrefab != nullptr, "CreatePrefabDocument: pSaveAsPrefab must be a valod object!");
  const ezRTTI* pRootType = pSaveAsPrefab->GetTypeAccessor().GetType();

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(szFile, true, pTypeDesc).Failed())
    return ezStatus(ezFmt("Document type is unknown: '{0}'", szFile));

  pTypeDesc->m_pManager->EnsureDocumentIsClosed(szFile);

  // prepare the current state as a graph
  ezAbstractObjectGraph PrefabGraph;

  ezDocumentObjectConverterWriter writer(&PrefabGraph, GetObjectManager());
  auto pPrefabGraphMainNode = writer.AddObjectToGraph(pSaveAsPrefab);

  PrefabGraph.ReMapNodeGuids(invPrefabSeed, true);

  ezDocument* pSceneDocument = nullptr;

  EZ_SUCCEED_OR_RETURN(pTypeDesc->m_pManager->CreateDocument(
    "Prefab", szFile, pSceneDocument, ezDocumentFlags::RequestWindow | ezDocumentFlags::AddToRecentFilesList));

  out_NewDocumentGuid = pSceneDocument->GetGuid();

  ezUuid rootGuid = pSaveAsPrefab->GetGuid();
  rootGuid.RevertCombinationWithSeed(invPrefabSeed);

  auto pPrefabSceneRoot = pSceneDocument->GetObjectManager()->GetRootObject();
  ezDocumentObject* pPrefabSceneMainObject = pSceneDocument->GetObjectManager()->CreateObject(pRootType, rootGuid);
  pSceneDocument->GetObjectManager()->AddObject(pPrefabSceneMainObject, pPrefabSceneRoot, "Children", 0);

  ezDocumentObjectConverterReader reader(
    &PrefabGraph, pSceneDocument->GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  reader.ApplyPropertiesToObject(pPrefabGraphMainNode, pPrefabSceneMainObject);

  pSceneDocument->SetModified(true);
  auto res = pSceneDocument->SaveDocument();
  pTypeDesc->m_pManager->CloseDocument(pSceneDocument);

  return res;
}


ezUuid ezDocument::ReplaceByPrefab(
  const ezDocumentObject* pRootObject, const char* szPrefabFile, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed)
{
  GetCommandHistory()->StartTransaction("Replace by Prefab");

  ezUuid instantiatedRoot;

  if (false) // create editor prefab
  {
    ezInstantiatePrefabCommand instCmd;
    instCmd.m_Index = pRootObject->GetParent()->GetChildIndex(pRootObject) - 1;
    instCmd.m_bAllowPickedPosition = false;
    instCmd.m_CreateFromPrefab = PrefabAsset;
    instCmd.m_Parent = pRootObject->GetParent() == GetObjectManager()->GetRootObject() ? ezUuid() : pRootObject->GetParent()->GetGuid();
    instCmd.m_sBasePrefabGraph = ezPrefabUtils::ReadDocumentAsString(
      szPrefabFile); // since the prefab might have been created just now, going through the cache (via GUID) will most likely fail
    instCmd.m_RemapGuid = PrefabSeed;

    GetCommandHistory()->AddCommand(instCmd);

    instantiatedRoot = instCmd.m_CreatedRootObject;
  }
  else // create an object with the reference prefab component
  {
    auto pHistory = GetCommandHistory();

    ezStringBuilder tmp;
    ezUuid CmpGuid;
    instantiatedRoot.CreateNewUuid();
    CmpGuid.CreateNewUuid();

    ezAddObjectCommand cmd;
    cmd.m_Parent = (pRootObject->GetParent() == GetObjectManager()->GetRootObject()) ? ezUuid() : pRootObject->GetParent()->GetGuid();
    cmd.m_Index = -1;
    cmd.SetType("ezGameObject");
    cmd.m_NewObjectGuid = instantiatedRoot;
    cmd.m_sParentProperty = "Children";

    EZ_VERIFY(pHistory->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    cmd.SetType("ezPrefabReferenceComponent");
    cmd.m_sParentProperty = "Components";
    cmd.m_Index = -1;
    cmd.m_NewObjectGuid = CmpGuid;
    cmd.m_Parent = instantiatedRoot;
    EZ_VERIFY(pHistory->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    ezSetObjectPropertyCommand cmd2;
    cmd2.m_Object = CmpGuid;
    cmd2.m_sProperty = "Prefab";
    cmd2.m_NewValue = ezConversionUtils::ToString(PrefabAsset, tmp).GetData();
    EZ_VERIFY(pHistory->AddCommand(cmd2).m_Result.Succeeded(), "AddCommand failed");
  }

  {
    ezRemoveObjectCommand remCmd;
    remCmd.m_Object = pRootObject->GetGuid();

    GetCommandHistory()->AddCommand(remCmd);
  }

  GetCommandHistory()->FinishTransaction();

  return instantiatedRoot;
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
  instCmd.m_Index = pObject->GetParent()->GetChildIndex(pObject);
  instCmd.m_bAllowPickedPosition = false;
  instCmd.m_CreateFromPrefab = PrefabAsset;
  instCmd.m_Parent = pObject->GetParent() == GetObjectManager()->GetRootObject() ? ezUuid() : pObject->GetParent()->GetGuid();
  instCmd.m_RemapGuid = pMeta->m_PrefabSeedGuid;
  instCmd.m_sBasePrefabGraph = ezPrefabCache::GetSingleton()->GetCachedPrefabDocument(pMeta->m_CreateFromPrefab);

  m_DocumentObjectMetaData.EndReadMetaData();

  pHistory->AddCommand(remCmd);
  pHistory->AddCommand(instCmd);

  return instCmd.m_CreatedRootObject;
}


void ezDocument::UpdatePrefabsRecursive(ezDocumentObject* pObject)
{
  // Deliberately copy the array as the UpdatePrefabObject function will add / remove elements from the array.
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
      // only recurse if no prefab was found
      // nested prefabs are not allowed
      UpdatePrefabsRecursive(pChild);
    }
  }
}

void ezDocument::UpdatePrefabObject(
  ezDocumentObject* pObject, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed, const char* szBasePrefab)
{
  const ezStringBuilder& sNewBasePrefab = ezPrefabCache::GetSingleton()->GetCachedPrefabDocument(PrefabAsset);

  ezStringBuilder sNewMergedGraph;
  ezPrefabUtils::Merge(szBasePrefab, sNewBasePrefab, pObject, true, PrefabSeed, sNewMergedGraph);

  // remove current object
  ezRemoveObjectCommand rm;
  rm.m_Object = pObject->GetGuid();

  // instantiate prefab again
  ezInstantiatePrefabCommand inst;
  inst.m_Index = pObject->GetParent()->GetChildIndex(pObject);
  inst.m_bAllowPickedPosition = false;
  inst.m_CreateFromPrefab = PrefabAsset;
  inst.m_Parent = pObject->GetParent() == GetObjectManager()->GetRootObject() ? ezUuid() : pObject->GetParent()->GetGuid();
  inst.m_RemapGuid = PrefabSeed;
  inst.m_sBasePrefabGraph = sNewBasePrefab;
  inst.m_sObjectGraph = sNewMergedGraph;

  GetCommandHistory()->AddCommand(rm);
  GetCommandHistory()->AddCommand(inst);
}

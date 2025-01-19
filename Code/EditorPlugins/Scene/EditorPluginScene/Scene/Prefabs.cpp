#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <ToolsFoundation/Command/TreeCommands.h>


void ezSceneDocument::UnlinkPrefabs(ezArrayPtr<const ezDocumentObject*> selection)
{
  SUPER::UnlinkPrefabs(selection);

  // Clear cached names.
  for (auto pObject : selection)
  {
    auto pMetaScene = m_GameObjectMetaData->BeginModifyMetaData(pObject->GetGuid());
    pMetaScene->m_CachedNodeName.Clear();
    m_GameObjectMetaData->EndModifyMetaData(ezGameObjectMetaData::CachedName);
  }
}


bool ezSceneDocument::IsObjectEditorPrefab(const ezUuid& object, ezUuid* out_pPrefabAssetGuid) const
{
  auto pMeta = m_DocumentObjectMetaData->BeginReadMetaData(object);
  const bool bIsPrefab = pMeta->m_CreateFromPrefab.IsValid();

  if (out_pPrefabAssetGuid)
  {
    *out_pPrefabAssetGuid = pMeta->m_CreateFromPrefab;
  }

  m_DocumentObjectMetaData->EndReadMetaData();

  return bIsPrefab;
}


bool ezSceneDocument::IsObjectEnginePrefab(const ezUuid& object, ezUuid* out_pPrefabAssetGuid) const
{
  const ezDocumentObject* pObject = GetObjectManager()->GetObject(object);

  ezHybridArray<ezVariant, 16> values;
  pObject->GetTypeAccessor().GetValues("Components", values);

  for (ezVariant& value : values)
  {
    auto pChild = GetObjectManager()->GetObject(value.Get<ezUuid>());

    // search for prefab components
    if (pChild->GetTypeAccessor().GetType()->IsDerivedFrom<ezPrefabReferenceComponent>())
    {
      ezVariant varPrefab = pChild->GetTypeAccessor().GetValue("Prefab");

      if (varPrefab.IsA<ezString>())
      {
        if (out_pPrefabAssetGuid)
        {
          const ezString sAsset = varPrefab.Get<ezString>();

          const auto info = ezAssetCurator::GetSingleton()->FindSubAsset(sAsset);

          if (info.isValid())
          {
            *out_pPrefabAssetGuid = info->m_Data.m_Guid;
          }
        }

        return true;
      }
    }
  }

  return false;
}

void ezSceneDocument::UpdatePrefabs()
{
  EZ_LOCK(m_GameObjectMetaData->GetMutex());
  SUPER::UpdatePrefabs();
}


ezUuid ezSceneDocument::ReplaceByPrefab(const ezDocumentObject* pRootObject, ezStringView sPrefabFile, const ezUuid& prefabAsset, const ezUuid& prefabSeed, bool bEnginePrefab)
{
  ezUuid newGuid = SUPER::ReplaceByPrefab(pRootObject, sPrefabFile, prefabAsset, prefabSeed, bEnginePrefab);
  if (newGuid.IsValid())
  {
    auto pMeta = m_GameObjectMetaData->BeginModifyMetaData(newGuid);
    pMeta->m_CachedNodeName.Clear();
    m_GameObjectMetaData->EndModifyMetaData(ezGameObjectMetaData::CachedName);
  }
  return newGuid;
}

ezUuid ezSceneDocument::RevertPrefab(const ezDocumentObject* pObject)
{
  auto pHistory = GetCommandHistory();
  const ezVec3 vLocalPos = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<ezVec3>();
  const ezQuat vLocalRot = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<ezQuat>();
  const ezVec3 vLocalScale = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>();
  const float fLocalUniformScale = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  ezUuid newGuid = SUPER::RevertPrefab(pObject);

  if (newGuid.IsValid())
  {
    ezSetObjectPropertyCommand setCmd;
    setCmd.m_Object = newGuid;

    setCmd.m_sProperty = "LocalPosition";
    setCmd.m_NewValue = vLocalPos;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalRotation";
    setCmd.m_NewValue = vLocalRot;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalScaling";
    setCmd.m_NewValue = vLocalScale;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalUniformScaling";
    setCmd.m_NewValue = fLocalUniformScale;
    pHistory->AddCommand(setCmd).AssertSuccess();
  }
  return newGuid;
}

void ezSceneDocument::UpdatePrefabObject(ezDocumentObject* pObject, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed, ezStringView sBasePrefab)
{
  auto pHistory = GetCommandHistory();
  const ezVec3 vLocalPos = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<ezVec3>();
  const ezQuat vLocalRot = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<ezQuat>();
  const ezVec3 vLocalScale = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>();
  const float fLocalUniformScale = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  SUPER::UpdatePrefabObject(pObject, PrefabAsset, PrefabSeed, sBasePrefab);

  // the root object has the same GUID as the PrefabSeed
  if (PrefabSeed.IsValid())
  {
    ezSetObjectPropertyCommand setCmd;
    setCmd.m_Object = PrefabSeed;

    setCmd.m_sProperty = "LocalPosition";
    setCmd.m_NewValue = vLocalPos;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalRotation";
    setCmd.m_NewValue = vLocalRot;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalScaling";
    setCmd.m_NewValue = vLocalScale;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalUniformScaling";
    setCmd.m_NewValue = fLocalUniformScale;
    pHistory->AddCommand(setCmd).AssertSuccess();
  }
}

void ezSceneDocument::ConvertToEditorPrefab(ezArrayPtr<const ezDocumentObject*> selection)
{
  ezDeque<const ezDocumentObject*> newSelection;

  auto pHistory = GetCommandHistory();
  pHistory->StartTransaction("Convert to Editor Prefab");

  for (const ezDocumentObject* pObject : selection)
  {
    ezUuid assetGuid;
    if (!IsObjectEnginePrefab(pObject->GetGuid(), &assetGuid))
      continue;

    auto pAsset = ezAssetCurator::GetSingleton()->GetSubAsset(assetGuid);

    if (!pAsset.isValid())
      continue;

    const ezTransform transform = GetGlobalTransform(pObject);

    ezUuid newGuid = ezUuid::MakeUuid();
    ezUuid newObject = ReplaceByPrefab(pObject, pAsset->m_pAssetInfo->m_Path.GetAbsolutePath(), assetGuid, newGuid, false);

    if (newObject.IsValid())
    {
      const ezDocumentObject* pNewObject = GetObjectManager()->GetObject(newObject);
      SetGlobalTransform(pNewObject, transform, TransformationChanges::All);

      newSelection.PushBack(pNewObject);
    }
  }

  pHistory->FinishTransaction();

  GetSelectionManager()->SetSelection(newSelection);
}

void ezSceneDocument::ConvertToEnginePrefab(ezArrayPtr<const ezDocumentObject*> selection)
{
  ezDeque<const ezDocumentObject*> newSelection;

  auto pHistory = GetCommandHistory();
  pHistory->StartTransaction("Convert to Engine Prefab");

  ezStringBuilder tmp;

  for (const ezDocumentObject* pObject : selection)
  {
    ezUuid assetGuid;
    if (!IsObjectEditorPrefab(pObject->GetGuid(), &assetGuid))
      continue;

    auto pAsset = ezAssetCurator::GetSingleton()->GetSubAsset(assetGuid);

    if (!pAsset.isValid())
      continue;

    const ezTransform transform = ComputeGlobalTransform(pObject);

    const ezDocumentObject* pNewObject = nullptr;

    // create an object with the reference prefab component
    {
      ezUuid ObjectGuid, CmpGuid;
      ObjectGuid = ezUuid::MakeUuid();
      CmpGuid = ezUuid::MakeUuid();

      ezAddObjectCommand cmd;
      cmd.m_Parent = (pObject->GetParent() == GetObjectManager()->GetRootObject()) ? ezUuid() : pObject->GetParent()->GetGuid();
      cmd.m_Index = pObject->GetPropertyIndex();
      cmd.SetType("ezGameObject");
      cmd.m_NewObjectGuid = ObjectGuid;
      cmd.m_sParentProperty = "Children";

      EZ_VERIFY(pHistory->AddCommand(cmd).Succeeded(), "AddCommand failed");

      cmd.SetType("ezPrefabReferenceComponent");
      cmd.m_sParentProperty = "Components";
      cmd.m_Index = -1;
      cmd.m_NewObjectGuid = CmpGuid;
      cmd.m_Parent = ObjectGuid;
      EZ_VERIFY(pHistory->AddCommand(cmd).Succeeded(), "AddCommand failed");

      ezSetObjectPropertyCommand cmd2;
      cmd2.m_Object = CmpGuid;
      cmd2.m_sProperty = "Prefab";
      cmd2.m_NewValue = ezConversionUtils::ToString(assetGuid, tmp).GetData();
      EZ_VERIFY(pHistory->AddCommand(cmd2).Succeeded(), "AddCommand failed");


      pNewObject = GetObjectManager()->GetObject(ObjectGuid);
    }

    // set same position
    SetGlobalTransform(pNewObject, transform, TransformationChanges::All);

    newSelection.PushBack(pNewObject);

    // delete old object
    {
      ezRemoveObjectCommand rem;
      rem.m_Object = pObject->GetGuid();

      EZ_VERIFY(pHistory->AddCommand(rem).Succeeded(), "AddCommand failed");
    }
  }

  pHistory->FinishTransaction();

  GetSelectionManager()->SetSelection(newSelection);
}

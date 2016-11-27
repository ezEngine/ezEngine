#include <PCH.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Dialogs/DuplicateDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <Core/World/GameObject.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Commands/SceneCommands.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <Core/World/GameObject.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <GameUtils/Components/PrefabReferenceComponent.h>


void ezSceneDocument::UnlinkPrefabs(const ezDeque<const ezDocumentObject*>& Selection)
{
  SUPER::UnlinkPrefabs(Selection);

  // Clear cached names.
  for (auto pObject : Selection)
  {
    auto pMetaScene = m_SceneObjectMetaData.BeginModifyMetaData(pObject->GetGuid());
    pMetaScene->m_CachedNodeName.Clear();
    m_SceneObjectMetaData.EndModifyMetaData(ezSceneObjectMetaData::CachedName);
  }
}


bool ezSceneDocument::IsObjectEditorPrefab(const ezUuid& object, ezUuid* out_PrefabAssetGuid) const
{
  auto pMeta = m_DocumentObjectMetaData.BeginReadMetaData(object);
  const bool bIsPrefab = pMeta->m_CreateFromPrefab.IsValid();

  if (out_PrefabAssetGuid)
  {
    *out_PrefabAssetGuid = pMeta->m_CreateFromPrefab;
  }

  m_DocumentObjectMetaData.EndReadMetaData();

  return bIsPrefab;
}


bool ezSceneDocument::IsObjectEnginePrefab(const ezUuid& object, ezUuid* out_PrefabAssetGuid) const
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
        if (out_PrefabAssetGuid)
        {
          const ezString sAsset = varPrefab.Get<ezString>();

          const auto info = ezAssetCurator::GetSingleton()->FindAssetInfo(sAsset);

          if (info.isValid())
          {
            *out_PrefabAssetGuid = info->m_Info.m_DocumentID;
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
  EZ_LOCK(m_SceneObjectMetaData.GetMutex());
  SUPER::UpdatePrefabs();
}


ezUuid ezSceneDocument::ReplaceByPrefab(const ezDocumentObject* pRootObject, const char* szPrefabFile, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed)
{
  ezUuid newGuid = SUPER::ReplaceByPrefab(pRootObject, szPrefabFile, PrefabAsset, PrefabSeed);
  if (newGuid.IsValid())
  {
    auto pMeta = m_SceneObjectMetaData.BeginModifyMetaData(newGuid);
    pMeta->m_CachedNodeName.Clear();
    m_SceneObjectMetaData.EndModifyMetaData(ezSceneObjectMetaData::CachedName);
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
    pHistory->AddCommand(setCmd);

    setCmd.m_sProperty = "LocalRotation";
    setCmd.m_NewValue = vLocalRot;
    pHistory->AddCommand(setCmd);

    setCmd.m_sProperty = "LocalScaling";
    setCmd.m_NewValue = vLocalScale;
    pHistory->AddCommand(setCmd);

    setCmd.m_sProperty = "LocalUniformScaling";
    setCmd.m_NewValue = fLocalUniformScale;
    pHistory->AddCommand(setCmd);
  }
  return newGuid;
}


void ezSceneDocument::ConvertToEditorPrefab(const ezDeque<const ezDocumentObject*>& Selection)
{
  auto pHistory = GetCommandHistory();
  pHistory->StartTransaction("Convert to Editor Prefab");

  for (const ezDocumentObject* pObject : Selection)
  {
    ezUuid assetGuid;
    if (!IsObjectEnginePrefab(pObject->GetGuid(), &assetGuid))
      continue;

    auto pAsset = ezAssetCurator::GetSingleton()->GetAssetInfo2(assetGuid);

    if (!pAsset.isValid())
      continue;

    const ezTransform transform = GetGlobalTransform(pObject);

    ezUuid newGuid;
    newGuid.CreateNewUuid();
    ezUuid newObject = ReplaceByPrefab(pObject, pAsset->m_sAbsolutePath, assetGuid, newGuid);

    const ezDocumentObject* pNewObject = GetObjectManager()->GetObject(newObject);
    SetGlobalTransform(pNewObject, transform, TransformationChanges::All);
  }

  pHistory->FinishTransaction();
}


void ezSceneDocument::ConvertToEnginePrefab(const ezDeque<const ezDocumentObject*>& Selection)
{
  auto pHistory = GetCommandHistory();
  pHistory->StartTransaction("Convert to Engine Prefab");

  for (const ezDocumentObject* pObject : Selection)
  {
    ezUuid assetGuid;
    if (!IsObjectEditorPrefab(pObject->GetGuid(), &assetGuid))
      continue;

    auto pAsset = ezAssetCurator::GetSingleton()->GetAssetInfo2(assetGuid);

    if (!pAsset.isValid())
      continue;

    const ezTransform transform = ComputeGlobalTransform(pObject);

    const ezDocumentObject* pNewObject = nullptr;

    // create an object with the reference prefab component
    {
      ezUuid ObjectGuid, CmpGuid;
      ObjectGuid.CreateNewUuid();
      CmpGuid.CreateNewUuid();

      ezAddObjectCommand cmd;
      cmd.m_Parent = (pObject->GetParent() == GetObjectManager()->GetRootObject()) ? ezUuid() : pObject->GetParent()->GetGuid();
      cmd.m_Index = -1;
      cmd.SetType("ezGameObject");
      cmd.m_NewObjectGuid = ObjectGuid;
      cmd.m_sParentProperty = "Children";

      EZ_VERIFY(pHistory->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

      cmd.SetType("ezPrefabReferenceComponent");
      cmd.m_sParentProperty = "Components";
      cmd.m_Index = -1;
      cmd.m_NewObjectGuid = CmpGuid;
      cmd.m_Parent = ObjectGuid;
      EZ_VERIFY(pHistory->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

      ezSetObjectPropertyCommand cmd2;
      cmd2.m_Object = CmpGuid;
      cmd2.m_sProperty = "Prefab";
      cmd2.m_NewValue = ezConversionUtils::ToString(assetGuid);
      EZ_VERIFY(pHistory->AddCommand(cmd2).m_Result.Succeeded(), "AddCommand failed");


      pNewObject = GetObjectManager()->GetObject(ObjectGuid);
    }

    // set same position
    SetGlobalTransform(pNewObject, transform, TransformationChanges::All);

    // delete old object
    {
      ezRemoveObjectCommand rem;
      rem.m_Object = pObject->GetGuid();

      EZ_VERIFY(pHistory->AddCommand(rem).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  pHistory->FinishTransaction();
}

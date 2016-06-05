#include <PCH.h>
#include <EditorPluginScene/DragDropHandlers/PrefabDragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPrefabComponentDragDropHandler, 1, ezRTTIDefaultAllocator<ezPrefabComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE


float ezPrefabComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (ezComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Prefab") ? 1.0f : 0.0f;
}

void ezPrefabComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_bShiftKeyDown)
  {
    if (pInfo->m_sTargetContext == "viewport")
      CreatePrefab(pInfo->m_vDropPosition, GetAssetGuid(pInfo), ezUuid(), -1);
    else
      CreatePrefab(pInfo->m_vDropPosition, GetAssetGuid(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);
  }
  else
  {
    if (pInfo->m_sTargetContext == "viewport")
      CreateDropObject(pInfo->m_vDropPosition, "ezPrefabReferenceComponent", "Prefab", GetAssetGuidString(pInfo), ezUuid(), -1);
    else
      CreateDropObject(pInfo->m_vDropPosition, "ezPrefabReferenceComponent", "Prefab", GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

void ezPrefabComponentDragDropHandler::CreatePrefab(const ezVec3& vPosition, const ezUuid& AssetGuid, ezUuid parent, ezInt32 iInsertChildIndex)
{
  ezVec3 vPos = vPosition;

  if (vPos.IsNaN())
    vPos.SetZero();

  ezSceneDocument* pScene = static_cast<ezSceneDocument*>(m_pDocument);
  auto pCmdHistory = m_pDocument->GetCommandHistory();

  ezInstantiatePrefabCommand PasteCmd;
  PasteCmd.m_Parent = parent;
  PasteCmd.m_CreateFromPrefab = AssetGuid;
  //PasteCmd.m_Index = iInsertChildIndex;
  PasteCmd.m_sJsonGraph = pScene->GetCachedPrefabDocument(AssetGuid);
  PasteCmd.m_RemapGuid.CreateNewUuid();

  if (PasteCmd.m_sJsonGraph.IsEmpty())
    return; // error

  pCmdHistory->AddCommand(PasteCmd);

  if (PasteCmd.m_CreatedRootObject.IsValid())
  {
    MoveObjectToPosition(PasteCmd.m_CreatedRootObject, vPos);

    m_DraggedObjects.PushBack(PasteCmd.m_CreatedRootObject);
  }
}

void ezPrefabComponentDragDropHandler::OnDragUpdate(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragUpdate(pInfo);
  
  // the way prefabs are instantiated on the runtime side means the selection is not always immediately 'correct'
  // by resetting the selection, we can fix this
  SelectCreatedObjects();
}

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

  CreatePrefab(pInfo->m_vDropPosition, GetAssetGuid(pInfo));
  SelectCreatedObjects();
  BeginTemporaryCommands();
}

void ezPrefabComponentDragDropHandler::CreatePrefab(const ezVec3& vPosition, const ezUuid& AssetGuid)
{
  ezSceneDocument* pScene = static_cast<ezSceneDocument*>(m_pDocument);
  auto pCmdHistory = m_pDocument->GetCommandHistory();

  ezInstantiatePrefabCommand PasteCmd;
  PasteCmd.m_sJsonGraph = pScene->GetCachedPrefabGraph(AssetGuid);
  PasteCmd.m_RemapGuid.CreateNewUuid();

  if (PasteCmd.m_sJsonGraph.IsEmpty())
    return; // error

  pCmdHistory->AddCommand(PasteCmd);

  if (PasteCmd.m_CreatedRootObject.IsValid())
  {
    auto pMeta = m_pDocument->m_DocumentObjectMetaData.BeginModifyMetaData(PasteCmd.m_CreatedRootObject);
    pMeta->m_CreateFromPrefab = AssetGuid;
    pMeta->m_PrefabSeedGuid = PasteCmd.m_RemapGuid;
    pMeta->m_sBasePrefab = PasteCmd.m_sJsonGraph;
    m_pDocument->m_DocumentObjectMetaData.EndModifyMetaData(ezDocumentObjectMetaData::PrefabFlag);

    MoveObjectToPosition(PasteCmd.m_CreatedRootObject, vPosition);

    m_DraggedObjects.PushBack(PasteCmd.m_CreatedRootObject);
  }
}

#include <EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/MeshDragDropHandler.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshComponentDragDropHandler, 1, ezRTTIDefaultAllocator<ezMeshComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

float ezMeshComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (ezComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Mesh") ? 1.0f : 0.0f;
}

void ezMeshComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "ezMeshComponent", "Mesh", GetAssetGuidString(pInfo), ezUuid(), -1);
  else
    CreateDropObject(pInfo->m_vDropPosition, "ezMeshComponent", "Mesh", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
                     pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimatedMeshComponentDragDropHandler, 1, ezRTTIDefaultAllocator<ezAnimatedMeshComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

float ezAnimatedMeshComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (ezComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Animated Mesh") ? 1.0f : 0.0f;
}

void ezAnimatedMeshComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "ezAnimatedMeshComponent", "Mesh", GetAssetGuidString(pInfo), ezUuid(), -1);
  else
    CreateDropObject(pInfo->m_vDropPosition, "ezAnimatedMeshComponent", "Mesh", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
                     pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

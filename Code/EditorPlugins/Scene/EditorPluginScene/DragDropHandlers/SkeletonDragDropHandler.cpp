#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/SkeletonDragDropHandler.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkeletonComponentDragDropHandler, 1, ezRTTIDefaultAllocator<ezSkeletonComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

float ezSkeletonComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (ezComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Skeleton") ? 1.0f : 0.0f;
}

void ezSkeletonComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "ezSkeletonComponent", "Skeleton", GetAssetGuidString(pInfo), pInfo->m_ActiveParentObject, -1);
  else
    CreateDropObject(pInfo->m_vDropPosition, "ezSkeletonComponent", "Skeleton", GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

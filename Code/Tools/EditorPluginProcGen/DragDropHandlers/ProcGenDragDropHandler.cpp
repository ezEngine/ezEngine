#include <EditorPluginProcGenPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginProcGen/DragDropHandlers/ProcGenDragDropHandler.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcPlacementComponentDragDropHandler, 1, ezRTTIDefaultAllocator<ezProcPlacementComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;


float ezProcPlacementComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (ezComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Procedural Placement") ? 1.0f : 0.0f;
}

void ezProcPlacementComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  ezUuid targetObject;
  ezInt32 iTargetInsertChildIndex = -1;

  if (pInfo->m_sTargetContext != "viewport")
  {
    targetObject = pInfo->m_TargetObject;
    iTargetInsertChildIndex = pInfo->m_iTargetObjectInsertChildIndex;
  }

  CreateDropObject(
    pInfo->m_vDropPosition, "ezProcPlacementComponent", "Resource", GetAssetGuidString(pInfo), targetObject, iTargetInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

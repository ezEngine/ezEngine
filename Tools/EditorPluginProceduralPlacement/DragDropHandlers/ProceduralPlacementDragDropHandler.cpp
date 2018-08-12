#include <PCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginProceduralPlacement/DragDropHandlers/ProceduralPlacementDragDropHandler.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProceduralPlacementComponentDragDropHandler, 1,
                                ezRTTIDefaultAllocator<ezProceduralPlacementComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;


float ezProceduralPlacementComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (ezComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Procedural Placement") ? 1.0f : 0.0f;
}

void ezProceduralPlacementComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "ezProceduralPlacementComponent", "Resource", GetAssetGuidString(pInfo), ezUuid(), -1);
  else
    CreateDropObject(pInfo->m_vDropPosition, "ezProceduralPlacementComponent", "Resource", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
                     pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

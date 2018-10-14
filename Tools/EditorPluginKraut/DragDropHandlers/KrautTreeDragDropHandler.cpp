#include <PCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginKraut/DragDropHandlers/KrautTreeDragDropHandler.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeComponentDragDropHandler, 1, ezRTTIDefaultAllocator<ezKrautTreeComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;


float ezKrautTreeComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (ezComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Kraut Tree") ? 1.0f : 0.0f;
}

void ezKrautTreeComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "ezKrautTreeComponent", "KrautTree", GetAssetGuidString(pInfo), ezUuid(), -1);
  else
    CreateDropObject(pInfo->m_vDropPosition, "ezKrautTreeComponent", "KrautTree", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
                     pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

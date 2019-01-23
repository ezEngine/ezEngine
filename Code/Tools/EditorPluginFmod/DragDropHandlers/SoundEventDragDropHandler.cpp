#include <PCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginFmod/DragDropHandlers/SoundEventDragDropHandler.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundEventComponentDragDropHandler, 1, ezRTTIDefaultAllocator<ezSoundEventComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;


float ezSoundEventComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (ezComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Sound Event") ? 1.0f : 0.0f;
}

void ezSoundEventComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "ezFmodEventComponent", "SoundEvent", GetAssetGuidString(pInfo), ezUuid(), -1);
  else
    CreateDropObject(pInfo->m_vDropPosition, "ezFmodEventComponent", "SoundEvent", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
                     pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

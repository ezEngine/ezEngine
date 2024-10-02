#include <EditorPluginFmod/EditorPluginFmodPCH.h>

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

  constexpr const char* szComponentType = "ezFmodEventComponent";
  constexpr const char* szPropertyName = "SoundEvent";

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, szComponentType, szPropertyName, GetAssetGuidString(pInfo), pInfo->m_ActiveParentObject, -1);
  }
  else
  {
    if (!pInfo->m_bCtrlKeyDown && pInfo->m_iTargetObjectInsertChildIndex == -1) // dropped directly on a node -> attach component only
    {
      AttachComponentToObject(szComponentType, szPropertyName, GetAssetGuidString(pInfo), pInfo->m_TargetObject);

      // make sure this object gets selected
      m_DraggedObjects.PushBack(pInfo->m_TargetObject);
    }
    else
    {
      CreateDropObject(pInfo->m_vDropPosition, szComponentType, szPropertyName, GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);
    }
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

#include <EditorPluginMiniAudio/EditorPluginMiniAudioPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginMiniAudio/DragDropHandlers/MiniAudioDragDropHandler.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMiniAudioSoundComponentDragDropHandler, 1, ezRTTIDefaultAllocator<ezMiniAudioSoundComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;


float ezMiniAudioSoundComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (ezComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "MiniAudioSound") ? 1.0f : 0.0f;
}

void ezMiniAudioSoundComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  constexpr const char* szComponentType = "ezMiniAudioSoundComponent";
  constexpr const char* szPropertyName = "Sound";

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

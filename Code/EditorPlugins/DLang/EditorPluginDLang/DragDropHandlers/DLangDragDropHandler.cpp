#include <EditorPluginDLang/EditorPluginDLangPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginDLang/DragDropHandlers/DLangDragDropHandler.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDLangComponentDragDropHandler, 1, ezRTTIDefaultAllocator<ezDLangComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;


float ezDLangComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (ezComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "DLang") ? 1.0f : 0.0f;
}

void ezDLangComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, "ezDLangComponent", "Script", GetAssetGuidString(pInfo), ezUuid(), -1);
  }
  else
  {
    if (pInfo->m_iTargetObjectInsertChildIndex == -1) // dropped directly on a node -> attach component only
    {
      AttachComponentToObject("ezDLangComponent", "Script", GetAssetGuidString(pInfo), pInfo->m_TargetObject);

      // make sure this object gets selected
      m_DraggedObjects.PushBack(pInfo->m_TargetObject);
    }
    else
    {
      CreateDropObject(pInfo->m_vDropPosition, "ezDLangComponent", "Script", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
        pInfo->m_iTargetObjectInsertChildIndex);
    }
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

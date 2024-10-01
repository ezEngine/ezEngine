#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginTypeScript/DragDropHandlers/TypeScriptDragDropHandler.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeScriptComponentDragDropHandler, 1, ezRTTIDefaultAllocator<ezTypeScriptComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;


float ezTypeScriptComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (ezComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "TypeScript") ? 1.0f : 0.0f;
}

void ezTypeScriptComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, "ezTypeScriptComponent", "Script", GetAssetGuidString(pInfo), pInfo->m_ActiveParentObject, -1);
  }
  else
  {
    if (pInfo->m_iTargetObjectInsertChildIndex == -1) // dropped directly on a node -> attach component only
    {
      AttachComponentToObject("ezTypeScriptComponent", "Script", GetAssetGuidString(pInfo), pInfo->m_TargetObject);

      // make sure this object gets selected
      m_DraggedObjects.PushBack(pInfo->m_TargetObject);
    }
    else
    {
      CreateDropObject(pInfo->m_vDropPosition, "ezTypeScriptComponent", "Script", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
        pInfo->m_iTargetObjectInsertChildIndex);
    }
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

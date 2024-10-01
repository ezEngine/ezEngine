#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/DecalDragDropHandler.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalComponentDragDropHandler, 1, ezRTTIDefaultAllocator<ezDecalComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;


float ezDecalComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (ezComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Decal") ? 1.0f : 0.0f;
}

void ezDecalComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  ezVariantArray var;
  var.PushBack(GetAssetGuidString(pInfo));

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, "ezDecalComponent", "Decals", var, pInfo->m_ActiveParentObject, -1);

    m_vAlignAxisWithNormal = -ezVec3::MakeAxisX();
  }
  else
    CreateDropObject(pInfo->m_vDropPosition, "ezDecalComponent", "Decals", var, pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

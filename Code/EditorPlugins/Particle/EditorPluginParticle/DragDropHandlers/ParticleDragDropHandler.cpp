#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginParticle/DragDropHandlers/ParticleDragDropHandler.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleComponentDragDropHandler, 1, ezRTTIDefaultAllocator<ezParticleComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;


float ezParticleComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (ezComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Particle Effect") ? 1.0f : 0.0f;
}

void ezParticleComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  constexpr const char* szComponentType = "ezParticleComponent";
  constexpr const char* szPropertyName = "Effect";

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

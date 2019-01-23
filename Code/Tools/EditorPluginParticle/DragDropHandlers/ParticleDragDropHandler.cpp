#include <PCH.h>

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

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "ezParticleComponent", "Effect", GetAssetGuidString(pInfo), ezUuid(), -1);
  else
    CreateDropObject(pInfo->m_vDropPosition, "ezParticleComponent", "Effect", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
                     pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

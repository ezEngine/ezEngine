#include <PCH.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmoRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezGizmoComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezHiddenAttribute(),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE

ezGizmoComponent::ezGizmoComponent()
{
  m_bUseDepthPrepass = false;
}

ezMeshRenderData* ezGizmoComponent::CreateRenderData(ezUInt32 uiBatchId) const
{
  ezGizmoRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezGizmoRenderData>(GetOwner(), uiBatchId);
  pRenderData->m_GizmoColor = m_GizmoColor;
  pRenderData->m_bUseDepthPrepass = m_bUseDepthPrepass;
  pRenderData->m_bIsPickable = m_bIsPickable;

  return pRenderData;
}


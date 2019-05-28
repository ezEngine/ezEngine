#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>

ezGizmoComponentManager::ezGizmoComponentManager(ezWorld* pWorld)
  : ezComponentManager(pWorld)
{
}

// clang-format off
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
EZ_END_COMPONENT_TYPE;
// clang-format on

ezGizmoComponent::ezGizmoComponent()
{
  m_bUseDepthPrepass = false;
}

ezMeshRenderData* ezGizmoComponent::CreateRenderData() const
{
  ezColor color = m_GizmoColor;

  auto pManager = static_cast<const ezGizmoComponentManager*>(GetOwningManager());
  if (GetUniqueID() == pManager->m_uiHighlightID)
  {
    color = ezColor(0.9f, 0.9f, 0.1f, color.a);
  }

  ezGizmoRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezGizmoRenderData>(GetOwner());
  pRenderData->m_GizmoColor = color;
  pRenderData->m_bUseDepthPrepass = m_bUseDepthPrepass;
  pRenderData->m_bIsPickable = m_bIsPickable;

  return pRenderData;
}

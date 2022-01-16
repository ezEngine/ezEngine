#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

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

ezGizmoComponent::ezGizmoComponent() = default;
ezGizmoComponent::~ezGizmoComponent() = default;

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

ezResult ezGizmoComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  ezResult r = SUPER::GetLocalBounds(bounds, bAlwaysVisible);

  // adjust the bounds to be mirrored and pretty large, to combat the constant size and face camera modes that are implemented by the shader
  //bounds.m_vBoxHalfExtends = (bounds.m_vCenter + bounds.m_vBoxHalfExtends) * 3.0f;
  //bounds.m_vCenter.SetZero();
  //bounds.m_fSphereRadius = ezMath::Max(bounds.m_vBoxHalfExtends.x, bounds.m_vBoxHalfExtends.y, bounds.m_vBoxHalfExtends.z);

  // since there is always only a single gizmo on screen, there's no harm in making it always visible
  bAlwaysVisible = true;
  return r;
}

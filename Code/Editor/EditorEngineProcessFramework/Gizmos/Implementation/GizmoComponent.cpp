#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

ezGizmoComponentManager::ezGizmoComponentManager(ezWorld* pWorld)
  : ezComponentManager(pWorld)
{
}

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmoRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezGizmoComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
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

ezMeshRenderData* ezGizmoComponent::CreateRenderData(const ezRenderDataManager* pRenderDataManager) const
{
  ezColor color = m_GizmoColor;

  auto pManager = static_cast<const ezGizmoComponentManager*>(GetOwningManager());
  if (GetUniqueID() == pManager->m_uiHighlightID)
  {
    color = ezColor(0.9f, 0.9f, 0.1f, color.a);
  }

  ezGizmoRenderData* pRenderData = pRenderDataManager->CreateRenderDataForThisFrame<ezGizmoRenderData>(GetOwner());
  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_GizmoColor = color;
  pRenderData->m_uiUniqueID = GetUniqueIdForRendering();
  pRenderData->m_bIsPickable = m_bIsPickable;

  return pRenderData;
}

void ezGizmoComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_Lines.IsEmpty())
  {
    ezTempHybridArray<ezDebugRendererLine, 64> lines;
    lines.Reserve(m_Lines.GetCount() / 2);
    for (ezUInt32 v = 0; v < m_Lines.GetCount(); v += 2)
    {
      auto& l = lines.ExpandAndGetRef();
      l.m_start = m_Lines[v];
      l.m_end = m_Lines[v + 1];
    }

    ezDebugRenderer::DrawLinesOccluded(ezDebugRendererContext(msg.m_pView->GetHandle()), lines, m_GizmoColor.GetDarker(), GetOwner()->GetGlobalTransform());
    ezDebugRenderer::DrawLines(ezDebugRendererContext(msg.m_pView->GetHandle()), lines, m_GizmoColor, GetOwner()->GetGlobalTransform());

    if (!m_hMesh.IsValid())
      return;
  }

  SUPER::OnMsgExtractRenderData(msg);
}

ezResult ezGizmoComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible, ezMsgUpdateLocalBounds& msg)
{
  ezResult r = EZ_SUCCESS;

  if (!m_Lines.IsEmpty())
  {
    // The actual line data lives in the component and can be updated without touching bounds.
    // A unit-sized placeholder is enough because the gizmo is flagged as always visible.
    bounds = ezBoundingBoxSphere::MakeFromCenterExtents(ezVec3(0), ezVec3(1), 1);
  }
  else
  {
    r = SUPER::GetLocalBounds(bounds, bAlwaysVisible, msg);
  }

  // Since there is always only a single gizmo on screen, there's no harm in making it always visible.
  // Must be set after SUPER, because the base implementation may reset it.
  bAlwaysVisible = true;
  return r;
}

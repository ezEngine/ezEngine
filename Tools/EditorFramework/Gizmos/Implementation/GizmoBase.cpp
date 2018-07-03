#include <PCH.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezGizmo::ezGizmo()
{
  m_bVisible = false;
  m_Transformation.SetIdentity();
  m_Transformation.m_vScale.SetZero();
}

void ezGizmo::SetVisible(bool bVisible)
{
  if (m_bVisible == bVisible)
    return;

  m_bVisible = bVisible;

  OnVisibleChanged(m_bVisible);
}

void ezGizmo::SetTransformation(const ezTransform& transform)
{
  if (m_Transformation.IsIdentical(transform))
    return;

  m_Transformation = transform;

  OnTransformationChanged(m_Transformation);
}


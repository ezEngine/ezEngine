#include <PCH.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmoBase, ezEditorInputContext, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezGizmoBase::ezGizmoBase()
{
  m_bVisible = false;
  m_Transformation.SetIdentity();
}

void ezGizmoBase::SetVisible(bool bVisible)
{
  if (m_bVisible == bVisible)
    return;

  m_bVisible = bVisible;

  OnVisibleChanged(m_bVisible);
}

void ezGizmoBase::SetTransformation(const ezMat4& transform)
{
  if (m_Transformation.IsIdentical(transform))
    return;

  m_Transformation = transform;

  OnTransformationChanged(m_Transformation);
}


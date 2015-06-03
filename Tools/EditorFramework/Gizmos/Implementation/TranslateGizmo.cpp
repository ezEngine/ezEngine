#include <PCH.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <Foundation/Logging/Log.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTranslateGizmo, ezGizmoBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTranslateGizmo::ezTranslateGizmo()
{
  m_AxisX.Configure(this, ezGizmoHandleType::Arrow, ezColorLinearUB(128, 0, 0));
  m_AxisY.Configure(this, ezGizmoHandleType::Arrow, ezColorLinearUB(0, 128, 0));
  m_AxisZ.Configure(this, ezGizmoHandleType::Arrow, ezColorLinearUB(0, 0, 128));

  SetVisible(false);
  SetTransformation(ezMat4::IdentityMatrix());
}

void ezTranslateGizmo::SetDocumentGuid(const ezUuid& guid)
{
  m_AxisX.SetDocumentGuid(guid);
  m_AxisY.SetDocumentGuid(guid);
  m_AxisZ.SetDocumentGuid(guid);
}

void ezTranslateGizmo::OnVisibleChanged(bool bVisible)
{
  m_AxisX.SetVisible(bVisible);
  m_AxisY.SetVisible(bVisible);
  m_AxisZ.SetVisible(bVisible);
}

void ezTranslateGizmo::OnTransformationChanged(const ezMat4& transform)
{
  ezMat4 m;

  m.SetRotationMatrixZ(ezAngle::Degree(-90));
  m_AxisX.SetTransformation(transform * m);

  m.SetIdentity();
  m_AxisY.SetTransformation(transform * m);

  m.SetRotationMatrixX(ezAngle::Degree(90));
  m_AxisZ.SetTransformation(transform * m);
}

bool ezTranslateGizmo::mousePressEvent(QMouseEvent* e)
{
  ezLog::Info("Clicked on Gizmo");

  SetActiveInputContext(this);

  return true;
}

bool ezTranslateGizmo::mouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return false;

  ezLog::Info("Released on Gizmo");

  SetActiveInputContext(nullptr);
  return true;
}

bool ezTranslateGizmo::mouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return false;

  return true;
}


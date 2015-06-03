#include <PCH.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>


ezTranslateGizmo::ezTranslateGizmo()
{
  m_AxisX.Configure(ezGizmoHandleType::Arrow, ezColorLinearUB(128, 0, 0));
  m_AxisY.Configure(ezGizmoHandleType::Arrow, ezColorLinearUB(0, 128, 0));
  m_AxisZ.Configure(ezGizmoHandleType::Arrow, ezColorLinearUB(0, 0, 128));

  SetVisible(false);
  SetTransformation(ezMat4::IdentityMatrix());
}

void ezTranslateGizmo::SetDocumentGuid(const ezUuid& guid)
{
  m_AxisX.SetDocumentGuid(guid);
  m_AxisY.SetDocumentGuid(guid);
  m_AxisZ.SetDocumentGuid(guid);
}

void ezTranslateGizmo::SetVisible(bool bVisible)
{
  m_AxisX.SetVisible(bVisible);
  m_AxisY.SetVisible(bVisible);
  m_AxisZ.SetVisible(bVisible);
}

void ezTranslateGizmo::SetTransformation(const ezMat4& transform)
{
  ezMat4 m;

  m.SetRotationMatrixZ(ezAngle::Degree(-90));
  m_AxisX.SetTransformation(transform * m);

  m.SetIdentity();
  m_AxisY.SetTransformation(transform * m);

  m.SetRotationMatrixX(ezAngle::Degree(90));
  m_AxisZ.SetTransformation(transform * m);
}



#include <PCH.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <Foundation/Logging/Log.h>
#include <QMouseEvent>
#include <CoreUtils/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>

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

void ezTranslateGizmo::FocusLost()
{
  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::EndInteractions;
  m_BaseEvents.Broadcast(ev);
}

bool ezTranslateGizmo::mousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return true;

  if (e->button() != Qt::MouseButton::LeftButton)
    return false;

  if (m_pInteractionGizmoHandle == &m_AxisX)
    m_vMoveAxis = m_AxisX.GetTransformation().GetColumn(1).GetAsVec3();
  else if (m_pInteractionGizmoHandle == &m_AxisY)
    m_vMoveAxis = m_AxisY.GetTransformation().GetColumn(1).GetAsVec3();
  else if (m_pInteractionGizmoHandle == &m_AxisZ)
    m_vMoveAxis = m_AxisZ.GetTransformation().GetColumn(1).GetAsVec3();
  else
    return false;

  m_LastInteraction = ezTime::Now();

  SetActiveInputContext(this);

  m_vLastPos = m_vInteractionPivot;

  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::BeginInteractions;
  m_BaseEvents.Broadcast(ev);

  return true;
}

bool ezTranslateGizmo::mouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return false;

  if (e->button() != Qt::MouseButton::LeftButton)
    return true;

  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::EndInteractions;
  m_BaseEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  msg.SendHighlightObjectMessage(GetDocumentWindow3D()->GetEditorEngineConnection());
  

  SetActiveInputContext(nullptr);
  return true;
}

bool ezTranslateGizmo::mouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return false;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::Seconds(1.0 / 30.0))
    return true;

  m_LastInteraction = tNow;

  ezMat4 mView, mProj, mViewProj, mInvViewProj;
  m_pCamera->GetViewMatrix(mView);
  m_pCamera->GetProjectionMatrix((float)m_Viewport.x / (float)m_Viewport.y, mProj);
  mViewProj = mProj * mView;
  mInvViewProj = mViewProj.GetInverse();

  ezVec3 vPos, vRayDir;
  if (ezGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, m_Viewport.x, m_Viewport.y, ezVec3(e->windowPos().x(), m_Viewport.y - e->windowPos().y(), 0), vPos, &vRayDir).Failed())
    return true;

  //ezLog::Info("Dir: %.2f | %.2f | %.2f", vRayDir.x, vRayDir.y, vRayDir.z);

  const ezVec3 vPlane1Normal = m_vMoveAxis.GetOrthogonalVector().GetNormalized();
  const ezVec3 vPlane2Normal = vPlane1Normal.Cross(m_vMoveAxis).GetNormalized();

  ezPlane Plane1, Plane2;
  Plane1.SetFromNormalAndPoint(vPlane1Normal, m_vInteractionPivot);
  Plane2.SetFromNormalAndPoint(vPlane2Normal, m_vInteractionPivot);

  float fIntersection1 = 0.0f;
  float fIntersection2 = 0.0f;

  bool bIntersect1 = Plane1.GetRayIntersection(m_pCamera->GetPosition(), vRayDir, &fIntersection1);
  bool bIntersect2 = Plane2.GetRayIntersection(m_pCamera->GetPosition(), vRayDir, &fIntersection2);

  if (!bIntersect1)
    fIntersection1 = ezMath::BasicType<float>::GetInfinity();
  if (!bIntersect2)
    fIntersection2 = ezMath::BasicType<float>::GetInfinity();

  if (!bIntersect1 && !bIntersect2)
    return true;

  const float fIntersection = ezMath::Min(fIntersection1, fIntersection2);
  const ezVec3 vIntersection = m_pCamera->GetPosition() + vRayDir * fIntersection;


  //ezLog::Info("Intersection: %.2f | %.2f | %.2f", vIntersection.x, vIntersection.y, vIntersection.z);

  ezVec3 vDiff(0);

  // Point on Ray
  {
    const ezVec3 vDirAlongRay = vIntersection - m_vInteractionPivot;
    const float fProjectedLength = vDirAlongRay.Dot(m_vMoveAxis);

    const ezVec3 vPosOnRay = m_vInteractionPivot + fProjectedLength * m_vMoveAxis;

    //ezLog::Info("vDiff: %.2f | %.2f | %.2f", vDiff.x, vDiff.y, vDiff.z);

    vDiff = vPosOnRay - m_vLastPos;
    m_vLastPos = vPosOnRay;
  }

  ezMat4 mTrans;
  mTrans.SetTranslationMatrix(vDiff);

  SetTransformation(mTrans * GetTransformation());

  BaseEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = BaseEvent::Type::Interaction;
  m_BaseEvents.Broadcast(ev);

  return true;
}


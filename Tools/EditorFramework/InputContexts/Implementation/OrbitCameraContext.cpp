#include <PCH.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <CoreUtils/Graphics/Camera.h>
#include <QKeyEvent>
#include <QDesktopWidget>

ezOrbitCameraContext::ezOrbitCameraContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  m_pCamera = nullptr;

  m_Mode = Mode::Off;

  SetOrbitVolume(ezVec3(0.0f), ezVec3(5.0f), ezVec3(-2, 0, 0));

  // while the camera moves, ignore all other shortcuts
  SetShortcutsDisabled(true);

  SetOwner(pOwnerWindow, pOwnerView);
}

void ezOrbitCameraContext::SetCamera(ezCamera* pCamera)
{
  if (m_pCamera == pCamera)
    return;

  m_pCamera = pCamera;
}

ezCamera* ezOrbitCameraContext::GetCamera() const
{
  return m_pCamera;
}

void ezOrbitCameraContext::SetOrbitVolume(const ezVec3& vCenterPos, const ezVec3& vHalfBoxSize, const ezVec3& vDefaultCameraPosition)
{
  m_vOrbitPoint = vCenterPos;
  m_Volume.SetCenterAndHalfExtents(vCenterPos, vHalfBoxSize);
  m_vDefaultCameraPosition = vDefaultCameraPosition;

  if (m_pCamera)
  {
    m_pCamera->LookAt(vDefaultCameraPosition, vCenterPos, ezVec3(0, 0, 1));
  }
}

void ezOrbitCameraContext::DoFocusLost(bool bCancel)
{
  m_Mode = Mode::Off;

  ResetCursor();
}

ezEditorInut ezOrbitCameraContext::DoMousePressEvent(QMouseEvent* e)
{
  if (m_pCamera == nullptr)
    return ezEditorInut::MayBeHandledByOthers;

  if (!m_pCamera->IsPerspective())
    return ezEditorInut::MayBeHandledByOthers;

  if (m_Mode == Mode::Off)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
    {
      m_Mode = Mode::Orbit;
      goto activate;
    }

    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_Mode = Mode::UpDown;
      goto activate;
    }

    if (e->button() == Qt::MouseButton::MiddleButton)
    {
      m_Mode = Mode::MovePlane;
      goto activate;
    }
  }

  if (m_Mode == Mode::Orbit)
  {
    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Pan;

    goto activate;
  }

  if (m_Mode == Mode::UpDown)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::Pan;

    goto activate;
  }

  return ezEditorInut::MayBeHandledByOthers;

activate:
  {
    m_LastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
    MakeActiveInputContext();
    return ezEditorInut::WasExclusivelyHandled;
  }
}

ezEditorInut ezOrbitCameraContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return ezEditorInut::MayBeHandledByOthers;

  if (m_Mode == Mode::Off)
    return ezEditorInut::MayBeHandledByOthers;


  if (m_Mode == Mode::Orbit)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::Off;
  }

  if (m_Mode == Mode::UpDown)
  {
    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Off;
  }

  if (m_Mode == Mode::MovePlane)
  {
    if (e->button() == Qt::MouseButton::MiddleButton)
      m_Mode = Mode::Off;
  }

  if (m_Mode == Mode::Pan)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::UpDown;

    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Orbit;
  }

  // just to be save
  if (e->buttons() == Qt::NoButton || m_Mode == Mode::Off)
  {
    m_Mode = Mode::Off;
    ResetCursor();
  }

  return ezEditorInut::WasExclusivelyHandled;
}

ezEditorInut ezOrbitCameraContext::DoMouseMoveEvent(QMouseEvent* e)
{
  // do nothing, unless this is an active context
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (!m_pCamera->IsPerspective())
    return ezEditorInut::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return ezEditorInut::MayBeHandledByOthers;

  if (m_Mode == Mode::Off)
    return ezEditorInut::MayBeHandledByOthers;

  const ezVec2I32 CurMousePos(e->globalX(), e->globalY());
  const ezVec2I32 diff = CurMousePos - m_LastMousePos;
  m_LastMousePos = UpdateMouseMode(e);

  SetCurrentMouseMode();

  const float fMouseMoveSensitivity = 0.002f;

  const ezVec3 vHalfExtents = m_Volume.GetHalfExtents();
  const float fMaxExtent = ezMath::Max(vHalfExtents.x, vHalfExtents.y, vHalfExtents.z);
  const float fSensitivity = 0.0001f * fMaxExtent;

  if (m_Mode == Mode::Orbit)
  {
    float fMoveRight = diff.x * fMouseMoveSensitivity;
    float fMoveUp = diff.y * fMouseMoveSensitivity;

    float fDistance = (m_vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength();

    m_pCamera->RotateLocally(ezAngle::Radian(0.0f), ezAngle::Radian(fMoveUp), ezAngle::Radian(0.0f));
    m_pCamera->RotateGlobally(ezAngle::Radian(0.0f), ezAngle::Radian(0.0f), ezAngle::Radian(fMoveRight));

    ezVec3 vDir = m_pCamera->GetDirForwards();
    if (fDistance == 0.0f || vDir.SetLength(fDistance).Failed())
    {
      vDir.Set(1.0f, 0, 0);
    }

    m_pCamera->LookAt(m_vOrbitPoint - vDir, m_vOrbitPoint, ezVec3(0.0f, 0.0f, 1.0f));
  }

  if (m_Mode == Mode::MovePlane)
  {
    const ezVec3 vRight = m_pCamera->GetCenterDirRight();
    ezVec3 vForward = m_pCamera->GetCenterDirForwards();
    vForward.z = 0;
    vForward.Normalize();

    ezVec3 vNewPos = m_vOrbitPoint + fSensitivity * diff.x * vRight - fSensitivity * diff.y * vForward;

    vNewPos = m_Volume.GetClampedPoint(vNewPos);
    const ezVec3 vCamDiff = vNewPos - m_vOrbitPoint;

    m_vOrbitPoint = vNewPos;
    m_pCamera->MoveGlobally(vCamDiff);
  }

  if (m_Mode == Mode::Pan)
  {
    const ezVec3 vRight = m_pCamera->GetCenterDirRight();
    const ezVec3 vUp = m_pCamera->GetCenterDirUp();

    ezVec3 vNewPos = m_vOrbitPoint + fSensitivity * diff.x * vRight - fSensitivity * diff.y * vUp;

    vNewPos = m_Volume.GetClampedPoint(vNewPos);
    const ezVec3 vCamDiff = vNewPos - m_vOrbitPoint;

    m_vOrbitPoint = vNewPos;
    m_pCamera->MoveGlobally(vCamDiff);
  }

  if (m_Mode == Mode::UpDown)
  {
    const ezVec3 vUp(0, 0, 1);

    ezVec3 vNewPos = m_vOrbitPoint - fSensitivity * diff.y * vUp;

    vNewPos = m_Volume.GetClampedPoint(vNewPos);
    const ezVec3 vCamDiff = vNewPos - m_vOrbitPoint;

    m_vOrbitPoint = vNewPos;
    m_pCamera->MoveGlobally(vCamDiff);
  }

  return ezEditorInut::WasExclusivelyHandled;
}

ezEditorInut ezOrbitCameraContext::DoWheelEvent(QWheelEvent* e)
{
  if (m_Mode != Mode::Off)
    return ezEditorInut::WasExclusivelyHandled; // ignore it, but others should not handle it either

  if (!m_pCamera->IsPerspective())
    return ezEditorInut::MayBeHandledByOthers;

  const float fScale = 1.1f;

  float fDistance = (m_vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength();
  if (e->delta() > 0)
  {
    fDistance /= fScale;
  }
  else
  {
    fDistance *= fScale;
  }

  ezVec3 vDir = m_pCamera->GetDirForwards();
  if (fDistance == 0.0f || vDir.SetLength(fDistance).Failed())
  {
    vDir.Set(1.0f, 0, 0);
  }

  m_pCamera->LookAt(m_vOrbitPoint - vDir, m_vOrbitPoint, ezVec3(0.0f, 0.0f, 1.0f));

  // handled, independent of whether we are the active context or not
  return ezEditorInut::WasExclusivelyHandled;
}


ezEditorInut ezOrbitCameraContext::DoKeyPressEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key_F)
  {
    if (!m_vOrbitPoint.IsEqual(m_Volume.GetCenter(), 0.1f))
    {
      const ezVec3 vDiff = m_Volume.GetCenter() - m_vOrbitPoint;
      m_vOrbitPoint = m_Volume.GetCenter();
      m_pCamera->MoveGlobally(vDiff);

      return ezEditorInut::WasExclusivelyHandled;
    }
  }

  if (e->key() == Qt::Key_Space)
  {
    if (m_vOrbitPoint.IsEqual(m_Volume.GetCenter(), 0.1f))
    {
      m_pCamera->LookAt(m_vDefaultCameraPosition, m_vOrbitPoint, ezVec3(0, 0, 1));

      return ezEditorInut::WasExclusivelyHandled;
    }
  }

  return ezEditorInut::MayBeHandledByOthers;
}

void ezOrbitCameraContext::ResetCursor()
{
  if (m_Mode == Mode::Off)
  {
    SetMouseMode(ezEditorInputContext::MouseMode::Normal);
    MakeActiveInputContext(false);
  }
}

void ezOrbitCameraContext::SetCurrentMouseMode()
{
  if (m_Mode != Mode::Off)
  {
    SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  }
  else
  {
    SetMouseMode(ezEditorInputContext::MouseMode::Normal);
  }
}

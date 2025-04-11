#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

ezOrbitCameraContext::ezOrbitCameraContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  m_Volume = ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3::MakeZero(), ezVec3::MakeZero());
  m_pCamera = nullptr;

  m_LastUpdate = ezTime::Now();

  // while the camera moves, ignore all other shortcuts
  SetShortcutsDisabled(true);

  SetOwner(pOwnerWindow, pOwnerView);
}

void ezOrbitCameraContext::SetCamera(ezCamera* pCamera)
{
  m_pCamera = pCamera;
}

ezCamera* ezOrbitCameraContext::GetCamera() const
{
  return m_pCamera;
}


void ezOrbitCameraContext::SetDefaultCameraRelative(const ezVec3& vDirection, float fDistanceScale)
{
  m_bFixedDefaultCamera = false;

  m_vDefaultCamera = vDirection;
  m_vDefaultCamera.NormalizeIfNotZero(ezVec3::MakeAxisX()).IgnoreResult();
  m_vDefaultCamera *= ezMath::Max(0.01f, fDistanceScale);
}

void ezOrbitCameraContext::SetDefaultCameraFixed(const ezVec3& vPosition)
{
  m_bFixedDefaultCamera = true;
  m_vDefaultCamera = vPosition;
}

void ezOrbitCameraContext::MoveCameraToDefaultPosition()
{
  if (!m_pCamera)
    return;

  const ezVec3 vCenterPos = m_Volume.GetCenter();
  ezVec3 vCamPos = m_vDefaultCamera;

  if (!m_bFixedDefaultCamera)
  {
    const ezVec3 ext = m_Volume.GetHalfExtents();

    vCamPos = vCenterPos + m_vDefaultCamera * ezMath::Max(0.1f, ezMath::Max(ext.x, ext.y, ext.z));
  }

  m_pCamera->LookAt(vCamPos, vCenterPos, ezVec3(0, 0, 1));
}

void ezOrbitCameraContext::SetOrbitVolume(const ezVec3& vCenterPos, const ezVec3& vHalfBoxSize)
{
  bool bSetCamLookAt = false;

  if (m_Volume.GetHalfExtents().IsZero() && !vHalfBoxSize.IsZero())
  {
    bSetCamLookAt = true;
  }

  m_Volume = ezBoundingBox::MakeFromCenterAndHalfExtents(vCenterPos, vHalfBoxSize);

  if (bSetCamLookAt)
  {
    MoveCameraToDefaultPosition();
  }
}

void ezOrbitCameraContext::DoFocusLost(bool bCancel)
{
  m_Mode = Mode::Off;

  m_bRun = false;
  m_bMoveForwards = false;
  m_bMoveBackwards = false;
  m_bMoveLeft = false;
  m_bMoveRight = false;
  m_bMoveUp = false;
  m_bMoveDown = false;

  ResetCursor();
}

ezEditorInput ezOrbitCameraContext::DoMousePressEvent(QMouseEvent* e)
{
  if (m_pCamera == nullptr)
    return ezEditorInput::MayBeHandledByOthers;

  if (!m_pCamera->IsPerspective())
    return ezEditorInput::MayBeHandledByOthers;

  if (m_Mode == Mode::Off)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
    {
      m_Mode = Mode::Orbit;
      goto activate;
    }

    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_Mode = Mode::Free;
      goto activate;
    }
  }

  if (m_Mode == Mode::Free)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::Pan;

    return ezEditorInput::WasExclusivelyHandled;
  }

  if (m_Mode == Mode::Orbit)
  {
    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Pan;

    return ezEditorInput::WasExclusivelyHandled;
  }

  return ezEditorInput::MayBeHandledByOthers;

activate:
{
  m_vLastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  MakeActiveInputContext();
  return ezEditorInput::WasExclusivelyHandled;
}
}

ezEditorInput ezOrbitCameraContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return ezEditorInput::MayBeHandledByOthers;

  if (m_Mode == Mode::Off)
    return ezEditorInput::MayBeHandledByOthers;


  if (m_Mode == Mode::Orbit)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::Off;
  }

  if (m_Mode == Mode::Free)
  {
    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Off;
  }

  if (m_Mode == Mode::Pan)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::Free;

    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Off;
  }

  // just to be save
  if (e->buttons() == Qt::NoButton || m_Mode == Mode::Off)
  {
    m_Mode = Mode::Off;
    m_bRun = false;
    m_bMoveForwards = false;
    m_bMoveBackwards = false;
    m_bMoveLeft = false;
    m_bMoveRight = false;
    m_bMoveUp = false;
    m_bMoveDown = false;
    ResetCursor();
  }

  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezOrbitCameraContext::DoMouseMoveEvent(QMouseEvent* e)
{
  // do nothing, unless this is an active context
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return ezEditorInput::MayBeHandledByOthers;

  if (!m_pCamera->IsPerspective())
    return ezEditorInput::MayBeHandledByOthers;

  if (m_Mode == Mode::Off)
    return ezEditorInput::MayBeHandledByOthers;

  const QSize viewSize = GetOwnerView()->size();

  const ezEditorPreferencesUser* pEditorPref = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();

  const ezVec2I32 CurMousePos(QCursor::pos().x(), QCursor::pos().y());
  const ezVec2I32 mouseDiff = CurMousePos - m_vLastMousePos;
  m_vLastMousePos = UpdateMouseMode(e);

  ezVec2 diffNorm = ezVec2(mouseDiff.x, mouseDiff.y);

  switch (m_pCamera->GetCameraMode())
  {
    case ezCameraMode::PerspectiveFixedFovX:
    case ezCameraMode::OrthoFixedWidth:
      diffNorm /= (float)viewSize.width();
      break;
    case ezCameraMode::PerspectiveFixedFovY:
    case ezCameraMode::OrthoFixedHeight:
      diffNorm /= (float)viewSize.height();
      break;

    default:
      break;
  }

  SetCurrentMouseMode();

  const float fMouseRotationSpeed = 2.0f * pEditorPref->m_fCameraRotationSpeed;

  const ezVec3 vHalfExtents = m_Volume.GetHalfExtents();
  const float fMaxExtent = ezMath::Max(vHalfExtents.x, vHalfExtents.y, vHalfExtents.z);
  const float fBoost = e->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier) ? 5.0f : 1.0f;

  if (m_Mode == Mode::Orbit)
  {
    const float fMoveRight = diffNorm.x;
    const float fMoveUp = -diffNorm.y;

    const ezVec3 vOrbitPoint = m_Volume.GetCenter();

    const float fDistance = (vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength();

    if (fDistance > 0.01f)
    {
      // first force the camera to rotate towards the orbit point
      // this way the camera position doesn't jump around
      m_pCamera->LookAt(m_pCamera->GetCenterPosition(), vOrbitPoint, ezVec3(0.0f, 0.0f, 1.0f));
    }

    // then rotate the camera, and adjust its position to again point at the orbit point

    m_pCamera->RotateLocally(ezAngle::MakeFromRadian(0.0f), ezAngle::MakeFromRadian(fMoveUp), ezAngle::MakeFromRadian(0.0f));
    m_pCamera->RotateGlobally(ezAngle::MakeFromRadian(0.0f), ezAngle::MakeFromRadian(0.0f), ezAngle::MakeFromRadian(fMoveRight));

    ezVec3 vDir = m_pCamera->GetDirForwards();
    if (fDistance == 0.0f || vDir.SetLength(fDistance).Failed())
    {
      vDir.Set(1.0f, 0, 0);
    }

    m_pCamera->LookAt(vOrbitPoint - vDir, vOrbitPoint, ezVec3(0.0f, 0.0f, 1.0f));
  }

  if (m_Mode == Mode::Free)
  {
    const float fAspectRatio = (float)viewSize.width() / (float)viewSize.height();
    const ezAngle fFovX = m_pCamera->GetFovX(fAspectRatio);
    const ezAngle fFovY = m_pCamera->GetFovY(fAspectRatio);

    const float fMouseRotateSensitivityX = fFovX.GetRadian() * fMouseRotationSpeed;
    const float fMouseRotateSensitivityY = fFovY.GetRadian() * fMouseRotationSpeed;

    float fRotateHorizontal = diffNorm.x * fMouseRotateSensitivityX;
    float fRotateVertical = -diffNorm.y * fMouseRotateSensitivityY;

    m_pCamera->RotateLocally(ezAngle::MakeFromRadian(0), ezAngle::MakeFromRadian(fRotateVertical), ezAngle::MakeFromRadian(0));
    m_pCamera->RotateGlobally(ezAngle::MakeFromRadian(0), ezAngle::MakeFromRadian(0), ezAngle::MakeFromRadian(fRotateHorizontal));
  }

  if (m_Mode == Mode::Pan)
  {
    const float fSpeedFactor = GetCameraSpeed();

    const float fMoveUp = -diffNorm.y * fSpeedFactor;
    const float fMoveRight = diffNorm.x * fSpeedFactor;

    m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);
  }

  return ezEditorInput::WasExclusivelyHandled;
}

ezEditorInput ezOrbitCameraContext::DoWheelEvent(QWheelEvent* e)
{
  if (m_Mode != Mode::Off)
    return ezEditorInput::WasExclusivelyHandled; // ignore it, but others should not handle it either

  if (!m_pCamera->IsPerspective())
    return ezEditorInput::MayBeHandledByOthers;

  const float fScale = e->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier) ? 1.4f : 1.1f;

  const ezVec3 vOrbitPoint = m_Volume.GetCenter();

  float fDistance = (vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength();

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  if (e->angleDelta().y() > 0)
#else
  if (e->delta() > 0)
#endif
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

  m_pCamera->LookAt(vOrbitPoint - vDir, vOrbitPoint, ezVec3(0.0f, 0.0f, 1.0f));

  // handled, independent of whether we are the active context or not
  return ezEditorInput::WasExclusivelyHandled;
}


ezEditorInput ezOrbitCameraContext::DoKeyPressEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key_F)
  {
    MoveCameraToDefaultPosition();
    return ezEditorInput::WasExclusivelyHandled;
  }

  if (m_Mode != Mode::Free)
    return ezEditorInput::MayBeHandledByOthers;

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;

  switch (e->key())
  {
    case Qt::Key_W:
      m_bMoveForwards = true;
      return ezEditorInput::WasExclusivelyHandled;
    case Qt::Key_S:
      m_bMoveBackwards = true;
      return ezEditorInput::WasExclusivelyHandled;
    case Qt::Key_A:
      m_bMoveLeft = true;
      return ezEditorInput::WasExclusivelyHandled;
    case Qt::Key_D:
      m_bMoveRight = true;
      return ezEditorInput::WasExclusivelyHandled;
    case Qt::Key_Q:
      m_bMoveDown = true;
      return ezEditorInput::WasExclusivelyHandled;
    case Qt::Key_E:
      m_bMoveUp = true;
      return ezEditorInput::WasExclusivelyHandled;
  }

  return ezEditorInput::MayBeHandledByOthers;
}

ezEditorInput ezOrbitCameraContext::DoKeyReleaseEvent(QKeyEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return ezEditorInput::MayBeHandledByOthers;

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;

  switch (e->key())
  {
    case Qt::Key_W:
      m_bMoveForwards = false;
      return ezEditorInput::WasExclusivelyHandled;
    case Qt::Key_S:
      m_bMoveBackwards = false;
      return ezEditorInput::WasExclusivelyHandled;
    case Qt::Key_A:
      m_bMoveLeft = false;
      return ezEditorInput::WasExclusivelyHandled;
    case Qt::Key_D:
      m_bMoveRight = false;
      return ezEditorInput::WasExclusivelyHandled;
    case Qt::Key_Q:
      m_bMoveDown = false;
      return ezEditorInput::WasExclusivelyHandled;
    case Qt::Key_E:
      m_bMoveUp = false;
      return ezEditorInput::WasExclusivelyHandled;
  }

  return ezEditorInput::MayBeHandledByOthers;
}

void ezOrbitCameraContext::UpdateContext()
{
  ezTime diff = ezTime::Now() - m_LastUpdate;
  m_LastUpdate = ezTime::Now();

  const double TimeDiff = ezMath::Min(diff.GetSeconds(), 0.1);

  float fSpeedFactor = TimeDiff;

  if (m_bRun)
    fSpeedFactor *= 5.0f;

  fSpeedFactor *= GetCameraSpeed();

  if (m_bMoveForwards)
    m_pCamera->MoveLocally(fSpeedFactor, 0, 0);
  if (m_bMoveBackwards)
    m_pCamera->MoveLocally(-fSpeedFactor, 0, 0);
  if (m_bMoveRight)
    m_pCamera->MoveLocally(0, fSpeedFactor, 0);
  if (m_bMoveLeft)
    m_pCamera->MoveLocally(0, -fSpeedFactor, 0);
  if (m_bMoveUp)
    m_pCamera->MoveGlobally(0, 0, 1 * fSpeedFactor);
  if (m_bMoveDown)
    m_pCamera->MoveGlobally(0, 0, -1 * fSpeedFactor);
}

float ezOrbitCameraContext::GetCameraSpeed() const
{
  const ezVec3 ext = m_Volume.GetHalfExtents();
  float fSize = ezMath::Max(0.1f, ext.x, ext.y, ext.z);

  return fSize;
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

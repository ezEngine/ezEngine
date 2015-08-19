#include <PCH.h>
#include <EditorPluginScene/InputContexts/CameraMoveContext.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <CoreUtils/Graphics/Camera.h>
#include <QKeyEvent>

static const float s_fMoveSpeed[31] =
{
  0.0078125f,
  0.01171875f,
  0.015625f,
  0.0234375f,
  0.03125f,

  0.046875f,
  0.0625f,
  0.09375f,
  0.125f,
  0.1875f,

  0.25f,
  0.375f,
  0.5f,
  0.75f,
  1.0f,

  1.5f,
  2.0f,
  3.0f,
  4.0f,
  6.0f,

  8.0f,
  12.0f,
  16.0f,
  24.0f,
  32.0f,

  48.0f,
  64.0f,
  96.0f,
  128.0f,
  192.0f,

  256.0f,
};

ezCameraMoveContext::ezCameraMoveContext(QWidget* pParentWidget, ezDocumentWindow3D* pOwner)
{
  m_pParentWidget = pParentWidget;

  m_pCamera = nullptr;
  m_fMoveSpeed = 1.0f;

  m_vOrbitPoint.SetZero();

  m_bRun = false;
  m_bSlowDown = false;
  m_bMoveForwards = false;
  m_bMoveBackwards = false;
  m_bMoveRight = false;
  m_bMoveLeft = false;
  m_bMoveUp = false;
  m_bMoveDown = false;
  m_bMoveForwardsInPlane = false;
  m_bMoveBackwardsInPlane = false;

  m_LastUpdate = ezTime::Now();

  m_bRotateCamera = false;
  m_bMoveCamera = false;
  m_bMoveCameraInPlane = false;
  m_bTempMousePosition = false;
  m_bOrbitCamera = false;


  // do not use SetMoveSpeed here, that would save that value to the settings
  m_iMoveSpeed = 15;
  m_fMoveSpeed = s_fMoveSpeed[m_iMoveSpeed];

  // while the camera moves, ignore all other shortcuts
  SetShortcutsDisabled(true);

  SetOwner(pOwner);
}

void ezCameraMoveContext::FocusLost()
{
  m_bRotateCamera = false;
  m_bMoveCamera = false;
  m_bMoveCameraInPlane = false;
  m_bOrbitCamera = false;

  ResetCursor();

  m_bRun = false;
  m_bSlowDown = false;
  m_bMoveForwards = false;
  m_bMoveBackwards = false;
  m_bMoveRight = false;
  m_bMoveLeft = false;
  m_bMoveUp = false;
  m_bMoveDown = false;
  m_bMoveForwardsInPlane = false;
  m_bMoveBackwardsInPlane = false;
}

void ezCameraMoveContext::LoadState()
{
  ezEditorApp::GetInstance()->GetDocumentSettings(GetOwner()->GetDocument()->GetDocumentPath(), "ScenePlugin").RegisterValueInt("CameraSpeed", 15, ezSettingsFlags::User);
  SetMoveSpeed(ezEditorApp::GetInstance()->GetDocumentSettings(GetOwner()->GetDocument()->GetDocumentPath(), "ScenePlugin").GetValueInt("CameraSpeed"));
}

void ezCameraMoveContext::UpdateContext()
{
  ezTime diff = ezTime::Now() - m_LastUpdate;
  m_LastUpdate = ezTime::Now();

  const double TimeDiff = ezMath::Min(diff.GetSeconds(), 0.1);

  float fSpeedFactor = m_fMoveSpeed * TimeDiff;

  if (m_bRun)
    fSpeedFactor *= 5.0f;
  if (m_bSlowDown)
    fSpeedFactor *= 0.2f;

  if (m_bMoveForwards)
    m_pCamera->MoveLocally(fSpeedFactor, 0, 0);
  if (m_bMoveBackwards)
    m_pCamera->MoveLocally(-fSpeedFactor, 0, 0);
  if (m_bMoveRight)
    m_pCamera->MoveLocally(0, fSpeedFactor, 0);
  if (m_bMoveLeft)
    m_pCamera->MoveLocally(0, -fSpeedFactor, 0);
  if (m_bMoveUp)
    m_pCamera->MoveGlobally(ezVec3(0, 0, 1) * fSpeedFactor);
  if (m_bMoveDown)
    m_pCamera->MoveGlobally(ezVec3(0, 0, -1) * fSpeedFactor);
  if (m_bMoveForwardsInPlane)
  {
    ezVec3 vDir = m_pCamera->GetCenterDirForwards();
    vDir.y = 0.0f;
    vDir.NormalizeIfNotZero(ezVec3::ZeroVector());
    m_pCamera->MoveGlobally(vDir * fSpeedFactor);
  }
  if (m_bMoveBackwardsInPlane)
  {
    ezVec3 vDir = m_pCamera->GetCenterDirForwards();
    vDir.y = 0.0f;
    vDir.NormalizeIfNotZero(ezVec3::ZeroVector());
    m_pCamera->MoveGlobally(vDir * -fSpeedFactor);
  }
}

bool ezCameraMoveContext::keyReleaseEvent(QKeyEvent* e)
{
  if (!IsActiveInputContext())
    return false;

  if (m_pCamera == nullptr)
    return false;

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;
  m_bSlowDown = (e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0;

  switch (e->key())
  {
  case Qt::Key_W:
    m_bMoveForwards = false;
    return true;
  case Qt::Key_S:
    m_bMoveBackwards = false;
    return true;
  case Qt::Key_A:
    m_bMoveLeft = false;
    return true;
  case Qt::Key_D:
    m_bMoveRight = false;
    return true;
  case Qt::Key_Q:
    m_bMoveUp = false;
    return true;
  case Qt::Key_E:
    m_bMoveDown = false;
    return true;
  case Qt::Key_Left:
    m_bMoveLeft = false;
    return true;
  case Qt::Key_Right:
    m_bMoveRight = false;
    return true;
  case Qt::Key_Up:
    m_bMoveForwardsInPlane = false;
    return true;
  case Qt::Key_Down:
    m_bMoveBackwardsInPlane = false;
    return true;
  }

  return false;
}

bool ezCameraMoveContext::keyPressEvent(QKeyEvent* e)
{
  if (m_pCamera == nullptr)
    return false;

  if (e->modifiers() == Qt::KeyboardModifier::ControlModifier)
    return false;

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;
  m_bSlowDown = (e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0;

  switch (e->key())
  {
  case Qt::Key_Left:
    m_bMoveLeft = true;
    SetActiveInputContext(this);
    return true;
  case Qt::Key_Right:
    m_bMoveRight = true;
    SetActiveInputContext(this);
    return true;
  case Qt::Key_Up:
    m_bMoveForwardsInPlane = true;
    SetActiveInputContext(this);
    return true;
  case Qt::Key_Down:
    m_bMoveBackwardsInPlane = true;
    SetActiveInputContext(this);
    return true;
  }

  if (!m_bRotateCamera)
    return false;

  switch (e->key())
  {
  case Qt::Key_W:
    m_bMoveForwards = true;
    return true;
  case Qt::Key_S:
    m_bMoveBackwards = true;
    return true;
  case Qt::Key_A:
    m_bMoveLeft = true;
    return true;
  case Qt::Key_D:
    m_bMoveRight = true;
    return true;
  case Qt::Key_Q:
    m_bMoveUp = true;
    return true;
  case Qt::Key_E:
    m_bMoveDown = true;
    return true;
  }

  return false;
}

bool ezCameraMoveContext::mousePressEvent(QMouseEvent* e)
{
  if (m_pCamera == nullptr)
    return false;

  if (e->button() == Qt::MouseButton::RightButton)
  {
    m_bRotateCamera = true;
    m_LastMousePos = e->globalPos();
    m_bDidMoveMouse[1] = false;
    MakeActiveInputContext();
    return true;
  }

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    m_bMoveCamera = true;
    m_LastMousePos = e->globalPos();
    m_bDidMoveMouse[0] = false;
    MakeActiveInputContext();
    return true;
  }

  if (e->button() == Qt::MouseButton::MiddleButton)
  {
    m_bOrbitCamera = false;
    m_bMoveCameraInPlane = false;

    if ((e->modifiers() & Qt::KeyboardModifier::ControlModifier) != 0)
      m_bOrbitCamera = true;
    else
      m_bMoveCameraInPlane = true;

    m_LastMousePos = e->globalPos();
    m_bDidMoveMouse[2] = false;
    MakeActiveInputContext();
    return true;
  }

  return false;
}

void ezCameraMoveContext::ResetCursor()
{
  if (!m_bRotateCamera && !m_bMoveCamera && !m_bMoveCameraInPlane && !m_bOrbitCamera)
  {
    if (m_bTempMousePosition)
    {
      m_bTempMousePosition = false;
      QCursor::setPos(m_OriginalMousePos);
    }

    m_pParentWidget->setCursor(QCursor(Qt::ArrowCursor));
    MakeActiveInputContext(false);
  }
}

void ezCameraMoveContext::SetBlankCursor()
{
  if (m_bRotateCamera || m_bMoveCamera || m_bMoveCameraInPlane || m_bOrbitCamera)
  {
    m_pParentWidget->setCursor(QCursor(Qt::BlankCursor));
  }
}

void ezCameraMoveContext::SetCursorToWindowCenter()
{
  if (!m_bTempMousePosition)
  {
    m_OriginalMousePos = m_LastMousePos;
    m_bTempMousePosition = true;
  }

  QSize size = m_pParentWidget->size();
  QPoint center;
  center.setX(size.width() / 2);
  center.setY(size.height() / 2);

  center = m_pParentWidget->mapToGlobal(center);

  m_LastMousePos = center;
  QCursor::setPos(center);
}

bool ezCameraMoveContext::mouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return false;

  if (m_pCamera == nullptr)
    return false;

  if (e->button() == Qt::MouseButton::RightButton)
  {
    m_bRotateCamera = false;

    m_bMoveForwards = false;
    m_bMoveBackwards = false;
    m_bMoveLeft = false;
    m_bMoveRight = false;
    m_bMoveUp = false;
    m_bMoveDown = false;

    ResetCursor();
    return true;
  }

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    m_bMoveCamera = false;
    ResetCursor();

    if (!m_bDidMoveMouse[0])
    {
      // not really handled, so make this context inactive and tell the surrounding code that it may pass
      // the event to the next handler
      return false;
    }

    return true;
  }

  if (e->button() == Qt::MouseButton::MiddleButton)
  {
    m_bMoveCameraInPlane = false;
    m_bOrbitCamera = false;

    ResetCursor();
    return true;
  }

  return false;
}

void ezCameraMoveContext::SetOrbitPoint(const ezVec3& vPos)
{
  m_vOrbitPoint = vPos;
}

bool ezCameraMoveContext::mouseMoveEvent(QMouseEvent* e)
{
  // do nothing, unless this is an active context
  if (!IsActiveInputContext())
    return false;

  // store that the mouse has been moved since the last click
  for (ezInt32 i = 0; i < EZ_ARRAY_SIZE(m_bDidMoveMouse); ++i)
    m_bDidMoveMouse[i] = true;

  // send a message to clear any highlight
  ezViewHighlightMsgToEngine msg;
  msg.SendHighlightObjectMessage(GetOwner()->GetEditorEngineConnection());

  if (m_pCamera == nullptr)
    return false;

  SetBlankCursor();

  float fBoost = 1.0f;
  float fRotateBoost = 1.0f;

  if (m_bRun)
    fBoost = 5.0f;
  if (m_bSlowDown)
  {
    fBoost = 0.1f;
    fRotateBoost = 0.2f;
  }

  const float fAspectRatio = (float)m_pParentWidget->size().width() / (float)m_pParentWidget->size().height();
  const ezAngle fFovX = m_pCamera->GetFovX(fAspectRatio);
  const ezAngle fFovY = m_pCamera->GetFovY(fAspectRatio);

  const float fMouseScale = 4.0f;

  const float fMouseMoveSensitivity = 0.002f * m_fMoveSpeed * fBoost;
  const float fMouseRotateSensitivityX = (fFovX.GetRadian() / (float)m_pParentWidget->size().width()) * fRotateBoost * fMouseScale;
  const float fMouseRotateSensitivityY = (fFovY.GetRadian() / (float)m_pParentWidget->size().height()) * fRotateBoost * fMouseScale;

  if (m_bRotateCamera && m_bMoveCamera) // left & right mouse button -> pan
  {
    const QPointF diff = e->globalPos() - m_LastMousePos;

    float fMoveUp = -diff.y() * fMouseMoveSensitivity;
    float fMoveRight = diff.x() * fMouseMoveSensitivity;

    m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);

    SetCursorToWindowCenter();
    return true;
  }

  if (m_bRotateCamera)
  {
    const QPointF diff = e->globalPos() - m_LastMousePos;

    float fRotateHorizontal = diff.x() * fMouseRotateSensitivityX;
    float fRotateVertical = diff.y() * fMouseRotateSensitivityY;

    m_pCamera->RotateLocally(ezAngle::Radian(0), ezAngle::Radian(fRotateVertical), ezAngle::Radian(0));
    m_pCamera->RotateGlobally(ezAngle::Radian(0), ezAngle::Radian(0), ezAngle::Radian(fRotateHorizontal));

    SetCursorToWindowCenter();
    return true;
  }

  if (m_bMoveCamera)
  {
    const QPointF diff = e->globalPos() - m_LastMousePos;

    float fMoveRight = diff.x() * fMouseMoveSensitivity;
    float fMoveForward = -diff.y() * fMouseMoveSensitivity;

    m_pCamera->MoveLocally(fMoveForward, fMoveRight, 0);

    SetCursorToWindowCenter();

    return true;
  }

  if (m_bMoveCameraInPlane)
  {
    const QPointF diff = e->globalPos() - m_LastMousePos;

    float fMoveRight = diff.x() * fMouseMoveSensitivity;
    float fMoveForward = -diff.y() * fMouseMoveSensitivity;

    m_pCamera->MoveLocally(0, fMoveRight, 0);

    ezVec3 vDir = m_pCamera->GetCenterDirForwards();
    vDir.z = 0.0f;
    vDir.NormalizeIfNotZero(ezVec3::ZeroVector());

    m_pCamera->MoveGlobally(vDir * fMoveForward);

    SetCursorToWindowCenter();

    return true;
  }

  if (m_bOrbitCamera)
  {
    const QPointF diff = e->globalPos() - m_LastMousePos;

    float fMoveRight = diff.x() * fMouseMoveSensitivity;
    float fMoveUp = -diff.y() * fMouseMoveSensitivity;

    const float fDistance = (m_vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength();

    m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);

    ezVec3 vDir = m_vOrbitPoint - m_pCamera->GetCenterPosition();
    if (fDistance == 0.0f || vDir.SetLength(fDistance).Failed())
    {
      vDir.Set(0.01f, 0, 0);
    }

    if (ezMath::Abs(vDir.GetNormalized().Dot(ezVec3(0,0,1))) < 0.999f)
      m_pCamera->LookAt(m_vOrbitPoint - vDir, m_vOrbitPoint, ezVec3(0.0f, 0.0f, 1.0f));
    else
      m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);

    SetCursorToWindowCenter();

    return true;
  }

  return false;
}

void ezCameraMoveContext::SetMoveSpeed(ezInt32 iSpeed)
{
  m_iMoveSpeed = ezMath::Clamp(iSpeed, 0, 30);
  m_fMoveSpeed = s_fMoveSpeed[m_iMoveSpeed];

  if (GetOwner()->GetDocument() != nullptr)
    ezEditorApp::GetInstance()->GetDocumentSettings(GetOwner()->GetDocument()->GetDocumentPath(), "ScenePlugin").SetValueInt("CameraSpeed", m_iMoveSpeed);
}

bool ezCameraMoveContext::wheelEvent(QWheelEvent* e)
{
  if (e->modifiers() == Qt::KeyboardModifier::ControlModifier)
  {

    if (e->delta() > 0)
    {
      SetMoveSpeed(m_iMoveSpeed + 1);
    }
    else
    {
      SetMoveSpeed(m_iMoveSpeed - 1);
    }

    // handled, independent of whether we are the active context or not
    return true;
  }

  {
    float fBoost = 0.25f;

    if (e->modifiers() == Qt::KeyboardModifier::ShiftModifier)
      fBoost *= 5.0f;
    if (e->modifiers() == Qt::KeyboardModifier::AltModifier)
      fBoost *= 0.1f;

    if (e->delta() > 0)
    {
      m_pCamera->MoveLocally(m_fMoveSpeed * fBoost, 0, 0);
    }
    else
    {
      m_pCamera->MoveLocally(-m_fMoveSpeed * fBoost, 0, 0);
    }

    // handled, independent of whether we are the active context or not
    return true;
  }
}


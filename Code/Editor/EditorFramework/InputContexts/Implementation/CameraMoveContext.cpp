#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>

static constexpr float s_fMoveSpeed[25] = {
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
  384.0f,

  512.0f,
  768.0f,
  1024.0f,
  1536.0f,
  2048.0f,
};

ezCameraMoveContext::ezCameraMoveContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  m_LastUpdate = ezTime::Now();

  // while the camera moves, ignore all other shortcuts
  SetShortcutsDisabled(true);

  SetOwner(pOwnerWindow, pOwnerView);
}

float ezCameraMoveContext::ConvertCameraSpeed(ezUInt32 uiSpeedIdx)
{
  return s_fMoveSpeed[ezMath::Clamp<ezUInt32>(uiSpeedIdx, 0, EZ_ARRAY_SIZE(s_fMoveSpeed) - 1)];
}

void ezCameraMoveContext::DoFocusLost(bool bCancel)
{
  m_bRotateCamera = false;
  m_bMoveCamera = false;
  m_bMoveCameraInPlane = false;
  m_bOrbitCamera = false;
  m_bSlideForwards = false;
  m_bOpenMenuOnMouseUp = false;
  m_bPanOrbitPoint = false;
  m_bPanCamera = false;

  ResetCursor();

  m_bRun = false;
  m_bMoveForwards = false;
  m_bMoveBackwards = false;
  m_bMoveRight = false;
  m_bMoveLeft = false;
  m_bMoveUp = false;
  m_bMoveDown = false;
  m_bMoveForwardsInPlane = false;
  m_bMoveBackwardsInPlane = false;
  m_bRotateLeft = false;
  m_bRotateRight = false;
  m_bRotateUp = false;
  m_bRotateDown = false;
}

void ezCameraMoveContext::LoadState()
{
  const ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetOwnerWindow()->GetDocument());
  SetMoveSpeed(pPreferences->GetCameraSpeed());
}

void ezCameraMoveContext::UpdateContext()
{
  ezTime diff = ezTime::Now() - m_LastUpdate;
  m_LastUpdate = ezTime::Now();

  const double TimeDiff = ezMath::Min(diff.GetSeconds(), 0.1);

  ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetOwnerWindow()->GetDocument());
  float fSpeedFactor = TimeDiff;

  if (m_bRun)
    fSpeedFactor *= 5.0f;

  const float fRotateHorizontal = 45 * fSpeedFactor;
  const float fRotateVertical = 45 * fSpeedFactor;

  fSpeedFactor *= ConvertCameraSpeed(pPreferences->GetCameraSpeed());

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
  if (m_bRotateLeft)
    m_pCamera->RotateGlobally(ezAngle::MakeFromRadian(0), ezAngle::MakeFromRadian(0), ezAngle::MakeFromDegree(-fRotateHorizontal));
  if (m_bRotateRight)
    m_pCamera->RotateGlobally(ezAngle::MakeFromRadian(0), ezAngle::MakeFromRadian(0), ezAngle::MakeFromDegree(fRotateHorizontal));
  if (m_bRotateUp)
    m_pCamera->RotateLocally(ezAngle::MakeFromRadian(0), ezAngle::MakeFromDegree(fRotateVertical), ezAngle::MakeFromRadian(0));
  if (m_bRotateDown)
    m_pCamera->RotateLocally(ezAngle::MakeFromRadian(0), ezAngle::MakeFromDegree(-fRotateVertical), ezAngle::MakeFromRadian(0));

  if (m_bMoveForwardsInPlane)
  {
    if (m_pCamera->IsPerspective())
    {
      ezVec3 vDir = m_pCamera->GetCenterDirForwards();
      vDir.z = 0.0f;
      vDir.NormalizeIfNotZero(ezVec3::MakeZero()).IgnoreResult();
      m_pCamera->MoveGlobally(vDir.x * fSpeedFactor, vDir.y * fSpeedFactor, vDir.z * fSpeedFactor);
    }
    else
    {
      m_pCamera->MoveLocally(0, 0, fSpeedFactor);
    }
  }

  if (m_bMoveBackwardsInPlane)
  {
    if (m_pCamera->IsPerspective())
    {
      ezVec3 vDir = m_pCamera->GetCenterDirForwards();
      vDir.z = 0.0f;
      vDir.NormalizeIfNotZero(ezVec3::MakeZero()).IgnoreResult();
      m_pCamera->MoveGlobally(vDir.x * -fSpeedFactor, vDir.y * -fSpeedFactor, vDir.z * -fSpeedFactor);
    }
    else
    {
      m_pCamera->MoveLocally(0, 0, -fSpeedFactor);
    }
  }
}

void ezCameraMoveContext::DeactivateIfLast()
{
  if (m_bRotateCamera || m_bMoveCamera || m_bMoveCameraInPlane || m_bOrbitCamera || m_bSlideForwards || m_bPanOrbitPoint || m_bMoveForwards || m_bMoveBackwards || m_bMoveRight || m_bMoveLeft || m_bMoveUp || m_bMoveDown || m_bMoveForwardsInPlane || m_bMoveBackwardsInPlane || m_bRotateLeft || m_bRotateRight || m_bRotateUp || m_bRotateDown || m_bPanCamera)
    return;

  FocusLost(false);
}

ezEditorInput ezCameraMoveContext::DoKeyReleaseEvent(QKeyEvent* e)
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
    case Qt::Key_Left:
      m_bMoveLeft = false;
      m_bRotateLeft = false;
      DeactivateIfLast();
      return ezEditorInput::WasExclusivelyHandled;
    case Qt::Key_Right:
      m_bMoveRight = false;
      m_bRotateRight = false;
      DeactivateIfLast();
      return ezEditorInput::WasExclusivelyHandled;
    case Qt::Key_Up:
      m_bMoveForwards = false;
      m_bRotateUp = false;
      DeactivateIfLast();
      return ezEditorInput::WasExclusivelyHandled;
    case Qt::Key_Down:
      m_bMoveBackwards = false;
      m_bRotateDown = false;
      DeactivateIfLast();
      return ezEditorInput::WasExclusivelyHandled;
  }

  return ezEditorInput::MayBeHandledByOthers;
}

ezEditorInput ezCameraMoveContext::DoKeyPressEvent(QKeyEvent* e)
{
  if (m_pCamera == nullptr)
    return ezEditorInput::MayBeHandledByOthers;

  // if (e->modifiers() == Qt::KeyboardModifier::ControlModifier)
  //   return ezEditorInput::MayBeHandledByOthers;

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;

  switch (e->key())
  {
    case Qt::Key_Left:
      if (e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
        m_bRotateLeft = true;
      else
        m_bMoveLeft = true;
      SetActiveInputContext(this);
      return ezEditorInput::WasExclusivelyHandled;
    case Qt::Key_Right:
      if (e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
        m_bRotateRight = true;
      else
        m_bMoveRight = true;
      SetActiveInputContext(this);
      return ezEditorInput::WasExclusivelyHandled;
    case Qt::Key_Up:
      if (e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
        m_bRotateUp = true;
      else
        m_bMoveForwards = true;
      SetActiveInputContext(this);
      return ezEditorInput::WasExclusivelyHandled;
    case Qt::Key_Down:
      if (e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
        m_bRotateDown = true;
      else
        m_bMoveBackwards = true;
      SetActiveInputContext(this);
      return ezEditorInput::WasExclusivelyHandled;
  }

  if (!m_bRotateCamera)
    return ezEditorInput::MayBeHandledByOthers;

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

ezEditorInput ezCameraMoveContext::DoMousePressEvent(QMouseEvent* e)
{
  if (m_pCamera == nullptr)
    return ezEditorInput::MayBeHandledByOthers;

  const QPoint curPos = QCursor::pos();
  m_vMouseClickPos.Set(curPos.x(), curPos.y());

  if (m_pCamera->IsOrthographic())
  {
    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_bOpenMenuOnMouseUp = (e->buttons() == Qt::MouseButton::RightButton);
      m_bMoveCamera = true;
      m_vLastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
      m_iDidMoveMouse[1] = 0;
      MakeActiveInputContext();
      return ezEditorInput::WasExclusivelyHandled;
    }
    else
    {
      m_bOpenMenuOnMouseUp = false;
    }
  }
  else
  {
    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_bSlideForwards = false;
      m_bRotateCamera = false;
      m_bOpenMenuOnMouseUp = (e->buttons() == Qt::MouseButton::RightButton);

      if ((e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0)
        m_bSlideForwards = true;
      else
        m_bRotateCamera = true;

      m_vLastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
      m_iDidMoveMouse[1] = 0;
      MakeActiveInputContext();
      return ezEditorInput::WasExclusivelyHandled;
    }
    else
    {
      m_bOpenMenuOnMouseUp = false;
    }

    if (e->button() == Qt::MouseButton::LeftButton)
    {
      m_bOrbitCamera = false;
      m_bPanCamera = false;

      if ((e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0)
      {
        m_bOrbitCamera = true;
        m_vLastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
      }
      else if (!e->modifiers().testAnyFlags(Qt::KeyboardModifier::ControlModifier | Qt::KeyboardModifier::ShiftModifier))
      {
        m_bPanCamera = true;
      }

      m_iDidMoveMouse[0] = 0;
      MakeActiveInputContext();
      return ezEditorInput::WasExclusivelyHandled;
    }

    if (e->button() == Qt::MouseButton::MiddleButton)
    {
      m_bRotateCamera = false;
      m_bMoveCamera = false;
      m_bMoveCameraInPlane = false;
      m_bPanOrbitPoint = false;
      m_bPanCamera = false;

      if (e->modifiers().testAnyFlags(Qt::KeyboardModifier::AltModifier))
      {
        m_bPanOrbitPoint = true;
      }
      else if (e->modifiers().testAnyFlags(Qt::KeyboardModifier::ControlModifier))
      {
        m_bMoveCamera = true;
      }
      else
      {
        m_bMoveCameraInPlane = true;
      }

      m_vLastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
      m_iDidMoveMouse[2] = 0;
      MakeActiveInputContext();
      return ezEditorInput::WasExclusivelyHandled;
    }
  }

  return ezEditorInput::MayBeHandledByOthers;
}

void ezCameraMoveContext::ResetCursor()
{
  if (!m_bRotateCamera && !m_bMoveCamera && !m_bMoveCameraInPlane && !m_bOrbitCamera && !m_bSlideForwards && !m_bPanCamera)
  {
    SetMouseMode(ezEditorInputContext::MouseMode::Normal);

    MakeActiveInputContext(false);
  }
}

void ezCameraMoveContext::SetCurrentMouseMode()
{
  if (m_bRotateCamera || m_bMoveCamera || m_bMoveCameraInPlane || m_bOrbitCamera || m_bSlideForwards)
  {
    SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  }
  else
  {
    SetMouseMode(ezEditorInputContext::MouseMode::Normal);
  }
}

ezEditorInput ezCameraMoveContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return ezEditorInput::MayBeHandledByOthers;

  if (m_pCamera->IsOrthographic())
  {
    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_bMoveCamera = false;

      ResetCursor();

      if (m_iDidMoveMouse[1] < 3 && m_bOpenMenuOnMouseUp)
      {
        GetOwnerView()->OpenContextMenu(e->globalPosition().toPoint());
      }
      return ezEditorInput::WasExclusivelyHandled;
    }
  }
  else
  {
    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_bRotateCamera = false;
      m_bSlideForwards = false;

      m_bMoveForwards = false;
      m_bMoveBackwards = false;
      m_bMoveLeft = false;
      m_bMoveRight = false;
      m_bMoveUp = false;
      m_bMoveDown = false;
      m_bRotateLeft = false;
      m_bRotateRight = false;
      m_bRotateUp = false;
      m_bRotateDown = false;

      ResetCursor();

      if (m_iDidMoveMouse[1] < 3 && m_bOpenMenuOnMouseUp)
      {
        GetOwnerView()->OpenContextMenu(e->globalPosition().toPoint());
      }

      return ezEditorInput::WasExclusivelyHandled;
    }

    if (e->button() == Qt::MouseButton::LeftButton)
    {
      m_bPanCamera = false;
      m_bOrbitCamera = false;
      ResetCursor();

      if (m_iDidMoveMouse[0] < 8) // be a bit lenient about how far the mouse may move before discarding the click
      {
        // not really handled, so make this context inactive and tell the surrounding code that it may pass
        // the event to the next handler
        return ezEditorInput::MayBeHandledByOthers;
      }

      return ezEditorInput::WasExclusivelyHandled;
    }

    if (e->button() == Qt::MouseButton::MiddleButton)
    {
      m_bRotateCamera = false;
      m_bMoveCamera = false;
      m_bMoveCameraInPlane = false;
      m_bPanOrbitPoint = false;

      ResetCursor();

      if (m_iDidMoveMouse[2] < 5)
      {
        // not really handled, so make this context inactive and tell the surrounding code that it may pass
        // the event to the next handler
        return ezEditorInput::MayBeHandledByOthers;
      }

      return ezEditorInput::WasExclusivelyHandled;
    }
  }

  return ezEditorInput::MayBeHandledByOthers;
}

ezVec3 ezCameraMoveContext::GetOrbitPoint() const
{
  return m_pCamera->GetCenterPosition() + m_pCamera->GetCenterDirForwards() * m_fOrbitPointDistance;
}

void ezCameraMoveContext::SetOrbitDistance(float fDistance)
{
  m_fOrbitPointDistance = fDistance;
}

ezEditorInput ezCameraMoveContext::DoMouseMoveEvent(QMouseEvent* e)
{
  // do nothing, unless this is an active context
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  const QSize viewSize = GetOwnerView()->size();

  const QPoint curPos = QCursor::pos();

  // store that the mouse has been moved since the last click
  for (ezInt32 i = 0; i < EZ_ARRAY_SIZE(m_iDidMoveMouse); ++i)
  {
    m_iDidMoveMouse[i] += ezMath::Abs(m_vMouseClickPos.x - curPos.x());
    m_iDidMoveMouse[i] += ezMath::Abs(m_vMouseClickPos.y - curPos.y());
  }

  m_vMouseClickPos.Set(curPos.x(), curPos.y());

  // send a message to clear any highlight
  ezViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  if (m_pCamera == nullptr)
    return ezEditorInput::MayBeHandledByOthers;

  const ezEditorPreferencesUser* pEditorPref = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
  const ezScenePreferencesUser* pScenePref = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetOwnerWindow()->GetDocument());

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;

  float fBoost = 1.0f;

  if (m_bRun)
    fBoost = 5.0f;

  const ezVec2I32 CurMousePos(QCursor::pos().x(), QCursor::pos().y());
  const ezVec2I32 mouseDiff = CurMousePos - m_vLastMousePos;
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

  if (m_pCamera->IsOrthographic())
  {
    if (m_bMoveCamera)
    {
      m_vLastMousePos = UpdateMouseMode(e);

      const float fDistPerPixel = m_pCamera->GetFovOrDim();
      const float fMoveUp = diffNorm.y * fDistPerPixel;
      const float fMoveRight = -diffNorm.x * fDistPerPixel;

      m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);

      return ezEditorInput::WasExclusivelyHandled;
    }
  }
  else
  {
    SetCurrentMouseMode();

    // correct the up vector, if it got messed up
    m_pCamera->LookAt(m_pCamera->GetCenterPosition(), m_pCamera->GetCenterPosition() + m_pCamera->GetCenterDirForwards(), ezVec3(0, 0, 1));

    const float fAspectRatio = (float)viewSize.width() / (float)viewSize.height();
    const ezAngle fFovX = m_pCamera->GetFovX(fAspectRatio);
    const ezAngle fFovY = m_pCamera->GetFovY(fAspectRatio);

    const float fMouseRotationSpeed = 2.0f * pEditorPref->m_fCameraRotationSpeed;

    const float fMouseMoveSensitivity = ConvertCameraSpeed(pScenePref->GetCameraSpeed()) * fBoost;
    const float fMouseRotateSensitivityX = fFovX.GetRadian() * fMouseRotationSpeed;
    const float fMouseRotateSensitivityY = fFovY.GetRadian() * fMouseRotationSpeed;

    if (m_bRotateCamera && m_bPanCamera) // left & right mouse button -> pan
    {
      float fMoveUp = -diffNorm.y * fMouseMoveSensitivity;
      float fMoveRight = diffNorm.x * fMouseMoveSensitivity;

      m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);

      m_vLastMousePos = UpdateMouseMode(e);
      return ezEditorInput::WasExclusivelyHandled;
    }

    if (m_bRotateCamera || m_bOrbitCamera)
    {
      float fDistToOrbit = 0.0f;
      ezVec3 vOrbitPoint;

      if (m_bOrbitCamera)
      {
        // fDistToOrbit = ezMath::Max(0.01f, (m_vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength());
        fDistToOrbit = ezMath::Max(0.01f, m_fOrbitPointDistance);
        vOrbitPoint = GetOrbitPoint();
      }

      float fRotateHorizontal = diffNorm.x * fMouseRotateSensitivityX;
      float fRotateVertical = -diffNorm.y * fMouseRotateSensitivityY;

      m_pCamera->RotateLocally(ezAngle::MakeFromRadian(0), ezAngle::MakeFromRadian(fRotateVertical), ezAngle::MakeFromRadian(0));
      m_pCamera->RotateGlobally(ezAngle::MakeFromRadian(0), ezAngle::MakeFromRadian(0), ezAngle::MakeFromRadian(fRotateHorizontal));

      if (m_bOrbitCamera)
      {
        const ezVec3 vDirection = m_pCamera->GetDirForwards();
        const ezVec3 vNewCamPos = vOrbitPoint - vDirection * fDistToOrbit;

        m_pCamera->LookAt(vNewCamPos, vOrbitPoint, m_pCamera->GetDirUp());
      }

      m_vLastMousePos = UpdateMouseMode(e);
      return ezEditorInput::WasExclusivelyHandled;
    }

    if (m_bMoveCamera)
    {
      float fMoveRight = diffNorm.x * fMouseMoveSensitivity;
      float fMoveForward = -diffNorm.y * fMouseMoveSensitivity;

      m_pCamera->MoveLocally(fMoveForward, fMoveRight, 0);

      m_vLastMousePos = UpdateMouseMode(e);

      return ezEditorInput::WasExclusivelyHandled;
    }

    if (m_bMoveCameraInPlane)
    {
      float fMoveRight = diffNorm.x * fMouseMoveSensitivity;
      float fMoveForward = -diffNorm.y * fMouseMoveSensitivity;

      m_pCamera->MoveLocally(0, fMoveRight, 0);

      ezVec3 vDir = m_pCamera->GetCenterDirForwards();
      vDir.z = 0.0f;
      vDir.NormalizeIfNotZero(ezVec3::MakeZero()).IgnoreResult();

      // m_vOrbitPoint += vDir * fMoveForward;
      m_pCamera->MoveGlobally(vDir.x * fMoveForward, vDir.y * fMoveForward, vDir.z * fMoveForward);

      m_vLastMousePos = UpdateMouseMode(e);

      return ezEditorInput::WasExclusivelyHandled;
    }

    if (m_bSlideForwards)
    {
      float fMove = diffNorm.y * fMouseMoveSensitivity * m_fOrbitPointDistance * 0.1f;
      const ezVec3 vOrbitPoint = GetOrbitPoint();

      m_pCamera->MoveLocally(fMove, 0, 0);

      m_vLastMousePos = UpdateMouseMode(e);

      m_fOrbitPointDistance = vOrbitPoint.GetDistanceTo(m_pCamera->GetCenterPosition());

      return ezEditorInput::WasExclusivelyHandled;
    }

    if (m_bPanOrbitPoint)
    {
      ezMat4 viewMatrix, projectionMatrix;
      GetOwnerView()->GetCameraMatrices(viewMatrix, projectionMatrix);

      ezMat4 mvp = projectionMatrix * viewMatrix;
      ezVec3 vOrbitPoint = GetOrbitPoint();

      ezVec3 vScreenPos(0);
      if (ezGraphicsUtils::ConvertWorldPosToScreenPos(mvp, 0, 0, viewSize.width(), viewSize.height(), vOrbitPoint, vScreenPos).Succeeded())
      {
        ezMat4 invMvp = mvp.GetInverse();

        vScreenPos.x -= mouseDiff.x;
        vScreenPos.y -= mouseDiff.y;

        ezVec3 vNewPoint(0);
        if (ezGraphicsUtils::ConvertScreenPosToWorldPos(invMvp, 0, 0, viewSize.width(), viewSize.height(), vScreenPos, vNewPoint).Succeeded())
        {
          const ezVec3 vDiff = vNewPoint - vOrbitPoint;

          m_pCamera->MoveGlobally(vDiff.x, vDiff.y, vDiff.z);
        }
      }

      m_vLastMousePos = UpdateMouseMode(e);
      return ezEditorInput::WasExclusivelyHandled;
    }
  }

  return ezEditorInput::MayBeHandledByOthers;
}

void ezCameraMoveContext::SetMoveSpeed(ezInt32 iSpeed)
{
  if (GetOwnerWindow()->GetDocument() != nullptr)
  {
    ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetOwnerWindow()->GetDocument());
    pPreferences->SetCameraSpeed(iSpeed);
  }
}

void ezCameraMoveContext::OnActivated()
{
  m_LastUpdate = ezTime::Now();
}

ezEditorInput ezCameraMoveContext::DoWheelEvent(QWheelEvent* e)
{
  const ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetOwnerWindow()->GetDocument());

  if (m_bMoveCamera || m_bMoveCameraInPlane || m_bOrbitCamera || m_bRotateCamera)
  {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    if (e->angleDelta().y() > 0)
#else
    if (e->delta() > 0)
#endif
    {
      SetMoveSpeed(pPreferences->GetCameraSpeed() + 1);
    }
    else
    {
      SetMoveSpeed(pPreferences->GetCameraSpeed() - 1);
    }
    return ezEditorInput::WasExclusivelyHandled; // ignore it, but others should not handle it either
  }

  if (m_pCamera->IsOrthographic())
  {
    float fBoost = 1.0f;
    const float fTick = 1.4f;

    float fNewDim = 20.0f;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    if (e->angleDelta().y() > 0)
#else
    if (e->delta() > 0)
#endif
      fNewDim = m_pCamera->GetFovOrDim() * ezMath::Pow(1.0f / fTick, fBoost);
    else
      fNewDim = m_pCamera->GetFovOrDim() * ezMath::Pow(fTick, fBoost);

    fNewDim = ezMath::Clamp(fNewDim, 1.0f, 2000.0f);

    m_pCamera->SetCameraMode(m_pCamera->GetCameraMode(), fNewDim, m_pCamera->GetNearPlane(), m_pCamera->GetFarPlane());

    // handled, independent of whether we are the active context or not
    return ezEditorInput::WasExclusivelyHandled;
  }
  else
  {
    if (e->modifiers() == Qt::KeyboardModifier::ControlModifier)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
      if (e->angleDelta().y() > 0)
#else
      if (e->delta() > 0)
#endif
      {
        SetMoveSpeed(pPreferences->GetCameraSpeed() + 1);
      }
      else
      {
        SetMoveSpeed(pPreferences->GetCameraSpeed() - 1);
      }

      // handled, independent of whether we are the active context or not
      return ezEditorInput::WasExclusivelyHandled;
    }

    {
      float fBoost = 0.25f;

      if (e->modifiers() == Qt::KeyboardModifier::ShiftModifier)
        fBoost *= 5.0f;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
      if (e->angleDelta().y() > 0)
#else
      if (e->delta() > 0)
#endif
      {
        m_pCamera->MoveLocally(ConvertCameraSpeed(pPreferences->GetCameraSpeed()) * fBoost, 0, 0);
      }
      else
      {
        m_pCamera->MoveLocally(-ConvertCameraSpeed(pPreferences->GetCameraSpeed()) * fBoost, 0, 0);
      }

      // handled, independent of whether we are the active context or not
      return ezEditorInput::WasExclusivelyHandled;
    }
  }
}

void ezCameraMoveContext::SetCamera(ezCamera* pCamera)
{
  if (m_pCamera == pCamera)
    return;

  m_pCamera = pCamera;
}

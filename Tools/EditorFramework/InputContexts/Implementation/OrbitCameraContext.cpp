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

  m_bOrbitCamera = false;
  m_vOrbitPoint.SetZero();
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

void ezOrbitCameraContext::SetOrbitPoint(const ezVec3& vPos)
{
  m_vOrbitPoint = vPos;
}


const ezVec3 ezOrbitCameraContext::GetOrbitPoint() const
{
  return m_vOrbitPoint;
}

void ezOrbitCameraContext::DoFocusLost(bool bCancel)
{
  m_bOrbitCamera = false;

  ResetCursor();
}

ezEditorInut ezOrbitCameraContext::DoMousePressEvent(QMouseEvent* e)
{
  if (m_pCamera == nullptr)
    return ezEditorInut::MayBeHandledByOthers;

  if (m_pCamera->GetCameraMode() != ezCameraMode::PerspectiveFixedFovX && m_pCamera->GetCameraMode() != ezCameraMode::PerspectiveFixedFovY)
    return ezEditorInut::MayBeHandledByOthers;

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    m_bOrbitCamera = true;

    m_LastMousePos = SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
    m_bDidMoveMouse[0] = false;
    MakeActiveInputContext();
    return ezEditorInut::WasExclusivelyHandled;
  }

  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezOrbitCameraContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return ezEditorInut::MayBeHandledByOthers;

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    m_bOrbitCamera = false;
    ResetCursor();

    if (!m_bDidMoveMouse[0])
    {
      // not really handled, so make this context inactive and tell the surrounding code that it may pass
      // the event to the next handler
      return ezEditorInut::MayBeHandledByOthers;
    }

    return ezEditorInut::WasExclusivelyHandled;
  }

  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezOrbitCameraContext::DoMouseMoveEvent(QMouseEvent* e)
{
  // do nothing, unless this is an active context
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (m_pCamera->GetCameraMode() != ezCameraMode::PerspectiveFixedFovX && m_pCamera->GetCameraMode() != ezCameraMode::PerspectiveFixedFovY)
    return ezEditorInut::MayBeHandledByOthers;

  // store that the mouse has been moved since the last click
  for (ezInt32 i = 0; i < EZ_ARRAY_SIZE(m_bDidMoveMouse); ++i)
    m_bDidMoveMouse[i] = true;

  if (m_pCamera == nullptr)
    return ezEditorInut::MayBeHandledByOthers;

  float fBoost = 1.0f;

  if (e->modifiers() == Qt::KeyboardModifier::AltModifier)
    fBoost *= 0.2f;

  const ezVec2I32 CurMousePos(e->globalX(), e->globalY());
  const ezVec2I32 diff = CurMousePos - m_LastMousePos;

  {
    SetCurrentMouseMode();

    const float fAspectRatio = (float)GetOwnerView()->size().width() / (float)GetOwnerView()->size().height();
    const ezAngle fFovX = m_pCamera->GetFovX(fAspectRatio);
    const ezAngle fFovY = m_pCamera->GetFovY(fAspectRatio);

    const float fMouseScale = 4.0f;

    const float fMouseMoveSensitivity = 0.002f * 1.0f * fBoost;
    const float fMouseRotateSensitivityX = (fFovX.GetRadian() / (float)GetOwnerView()->size().width()) * fBoost * fMouseScale;
    const float fMouseRotateSensitivityY = (fFovY.GetRadian() / (float)GetOwnerView()->size().height()) * fBoost * fMouseScale;

    if (m_bOrbitCamera)
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

      m_LastMousePos = UpdateMouseMode(e);

      return ezEditorInut::WasExclusivelyHandled;
    }
  }

  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezOrbitCameraContext::DoWheelEvent(QWheelEvent* e)
{
  if (m_bOrbitCamera)
    return ezEditorInut::WasExclusivelyHandled; // ignore it, but others should not handle it either

  if (m_pCamera->GetCameraMode() != ezCameraMode::PerspectiveFixedFovX && m_pCamera->GetCameraMode() != ezCameraMode::PerspectiveFixedFovY)
    return ezEditorInut::MayBeHandledByOthers;

  float fScale = 1.1f;

  if (e->modifiers() == Qt::KeyboardModifier::AltModifier)
    fScale = 1.01f;

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

void ezOrbitCameraContext::ResetCursor()
{
  if (!m_bOrbitCamera)
  {
    SetMouseMode(ezEditorInputContext::MouseMode::Normal);
    MakeActiveInputContext(false);
  }
}

void ezOrbitCameraContext::SetCurrentMouseMode()
{
  if (m_bOrbitCamera)
  {
    SetMouseMode(ezEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  }
  else
  {
    SetMouseMode(ezEditorInputContext::MouseMode::Normal);
  }
}

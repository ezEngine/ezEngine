#pragma once

#include <Foundation/Time/Time.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <QPoint>

class ezCamera;

struct ezCameraMoveContextSettings
{
  ezCameraMoveContextSettings()
  {
    m_vOrbitPoint.SetZero();
  }

  ezVec3 m_vOrbitPoint;
};

class ezCameraMoveContext : public ezEditorInputContext
{
public:
  ezCameraMoveContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, ezCameraMoveContextSettings* pSettings);

  void LoadState();

  void SetCamera(ezCamera* pCamera);

  void SetOrbitPoint(const ezVec3& vPos);

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInut DoKeyPressEvent(QKeyEvent* e) override;
  virtual ezEditorInut DoKeyReleaseEvent(QKeyEvent* e) override;
  virtual ezEditorInut DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseMoveEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoWheelEvent(QWheelEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override {}

private:
  virtual void UpdateContext() override;

  void SetMoveSpeed(ezInt32 iSpeed);
  void ResetCursor();
  void SetCurrentMouseMode();

  ezCameraMoveContextSettings* m_pSettings;

  ezVec2I32 m_LastMousePos;

  bool m_bRotateCamera;
  bool m_bMoveCamera;
  bool m_bMoveCameraInPlane;
  bool m_bOrbitCamera;
  bool m_bSlideForwards;
  bool m_bPanOrbitPoint;
  float m_fSlideForwardsDistance;

  ezCamera* m_pCamera;

  bool m_bRun;
  bool m_bSlowDown;
  bool m_bMoveForwards;
  bool m_bMoveBackwards;
  bool m_bMoveRight;
  bool m_bMoveLeft;
  bool m_bMoveUp;
  bool m_bMoveDown;
  bool m_bMoveForwardsInPlane;
  bool m_bMoveBackwardsInPlane;
  bool m_bDidMoveMouse[3]; // Left Click, Right Click, Middle Click

  ezTime m_LastUpdate;
};
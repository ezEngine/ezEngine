#pragma once

#include <Foundation/Time/Time.h>
#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>
#include <QPoint>

class ezCamera;

struct ezCameraMoveContextSettings
{
  ezCameraMoveContextSettings()
  {
    m_vOrbitPoint.SetZero();
    m_iMoveSpeed = 15;
  }

  ezInt32 m_iMoveSpeed;
  ezVec3 m_vOrbitPoint;
};

class ezCameraMoveContext : public ezEditorInputContext
{
public:
  ezCameraMoveContext(ezDocumentWindow3D* pOwnerWindow, ezEngineViewWidget* pOwnerView, ezCameraMoveContextSettings* pSettings);

  virtual void FocusLost() override;

  void LoadState();

  virtual bool keyPressEvent(QKeyEvent* e) override;
  virtual bool keyReleaseEvent(QKeyEvent* e) override;
  virtual bool mousePressEvent(QMouseEvent* e) override;
  virtual bool mouseReleaseEvent(QMouseEvent* e) override;
  virtual bool mouseMoveEvent(QMouseEvent* e) override;
  virtual bool wheelEvent(QWheelEvent* e) override;

  void SetCamera(ezCamera* pCamera) { m_pCamera = pCamera; }

  void SetOrbitPoint(const ezVec3& vPos);

protected:
  virtual void OnSetOwner(ezDocumentWindow3D* pOwnerWindow, ezEngineViewWidget* pOwnerView) override {}

private:
  virtual void UpdateContext() override;

  void SetMoveSpeed(ezInt32 iSpeed);
  void ResetCursor();
  void SetBlankCursor();
  void SetCursorToWindowCenter();

  ezCameraMoveContextSettings* m_pSettings;

  QPoint m_LastMousePos;
  QPoint m_OriginalMousePos;
  bool m_bTempMousePosition;

  bool m_bRotateCamera;
  bool m_bMoveCamera;
  bool m_bMoveCameraInPlane;
  bool m_bOrbitCamera;

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
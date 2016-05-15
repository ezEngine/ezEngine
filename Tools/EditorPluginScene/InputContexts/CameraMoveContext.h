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
  }

  ezVec3 m_vOrbitPoint;
};

class ezCameraMoveContext : public ezEditorInputContext
{
public:
  ezCameraMoveContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, ezCameraMoveContextSettings* pSettings);

  virtual void FocusLost(bool bCancel) override;

  void LoadState();

  virtual ezEditorInut keyPressEvent(QKeyEvent* e) override;
  virtual ezEditorInut keyReleaseEvent(QKeyEvent* e) override;
  virtual ezEditorInut mousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut mouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInut mouseMoveEvent(QMouseEvent* e) override;
  virtual ezEditorInut wheelEvent(QWheelEvent* e) override;

  void SetCamera(ezCamera* pCamera);

  void SetOrbitPoint(const ezVec3& vPos);

protected:
  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override {}

private:
  virtual void UpdateContext() override;

  void SetMoveSpeed(ezInt32 iSpeed);
  void ResetCursor();
  void SetBlankCursor();
  void SetCursorToWindowCenter(QPoint pos);

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
#pragma once

#include <Foundation/Time/Time.h>
#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>
#include <QPoint>

class ezCamera;

class ezCameraMoveContext : public ezEditorInputContext
{
public:
  ezCameraMoveContext(QWidget* pParentWidget, ezDocumentBase* pDocument, ezDocumentWindow3D* pDocumentWindow);

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

private:
  virtual void UpdateContext() override;

  QWidget* m_pParentWidget;
  ezDocumentBase* m_pDocument;
  ezDocumentWindow3D* m_pDocumentWindow;

  void SetMoveSpeed(ezInt32 iSpeed);
  void ResetCursor();
  void SetBlankCursor();
  void SetCursorToWindowCenter();

  QPoint m_LastMousePos;
  QPoint m_OriginalMousePos;
  bool m_bTempMousePosition;

  bool m_bRotateCamera;
  bool m_bMoveCamera;
  bool m_bMoveCameraInPlane;
  bool m_bOrbitCamera;
  ezInt32 m_iMoveSpeed;

  ezCamera* m_pCamera;
  float m_fMoveSpeed;

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
  ezVec3 m_vOrbitPoint;

  ezTime m_LastUpdate;
};
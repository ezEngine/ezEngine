#pragma once

#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <Foundation/Time/Time.h>
#include <QPoint>

class ezCamera;

class EZ_EDITORFRAMEWORK_DLL ezCameraMoveContext : public ezEditorInputContext
{
public:
  ezCameraMoveContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView);

  void LoadState();

  void SetCamera(ezCamera* pCamera);

  const ezVec3& GetOrbitPoint() const;
  void SetOrbitPoint(const ezVec3& vPos);

  static float ConvertCameraSpeed(ezUInt32 uiSpeedIdx);

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual ezEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;
  virtual ezEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoWheelEvent(QWheelEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override {}


  void OnActivated() override;

private:
  virtual void UpdateContext() override;

  void SetMoveSpeed(ezInt32 iSpeed);
  void ResetCursor();
  void SetCurrentMouseMode();
  void DeactivateIfLast();

  ezVec3 m_vOrbitPoint;

  ezVec2I32 m_vLastMousePos;

  bool m_bRotateCamera;
  bool m_bMoveCamera;
  bool m_bMoveCameraInPlane;
  bool m_bOrbitCamera;
  bool m_bSlideForwards;
  bool m_bPanOrbitPoint;
  float m_fSlideForwardsDistance;
  bool m_bOpenMenuOnMouseUp;

  ezCamera* m_pCamera;

  bool m_bRun = false;
  bool m_bSlowDown = false;
  bool m_bMoveForwards = false;
  bool m_bMoveBackwards = false;
  bool m_bMoveRight = false;
  bool m_bMoveLeft = false;
  bool m_bMoveUp = false;
  bool m_bMoveDown = false;
  bool m_bMoveForwardsInPlane = false;
  bool m_bMoveBackwardsInPlane = false;
  bool m_bDidMoveMouse[3] = {false, false, false}; // Left Click, Right Click, Middle Click

  bool m_bRotateLeft = false;
  bool m_bRotateRight = false;
  bool m_bRotateUp = false;
  bool m_bRotateDown = false;

  ezTime m_LastUpdate;
};

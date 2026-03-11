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

  ezVec3 GetOrbitPoint() const;
  void SetOrbitDistance(float fDistance);

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

  float m_fOrbitPointDistance = 1.0f;

  ezVec2I32 m_vLastMousePos = ezVec2I32::MakeZero();
  ezVec2I32 m_vMouseClickPos = ezVec2I32::MakeZero();

  bool m_bRotateCamera = false;
  bool m_bMoveCamera = false;
  bool m_bMoveCameraInPlane = false;
  bool m_bOrbitCamera = false;
  bool m_bSlideForwards = false;
  bool m_bPanOrbitPoint = false;
  bool m_bPanCamera = false;
  bool m_bOpenMenuOnMouseUp = false;

  ezCamera* m_pCamera = nullptr;

  bool m_bRun = false;
  bool m_bMoveForwards = false;
  bool m_bMoveBackwards = false;
  bool m_bMoveRight = false;
  bool m_bMoveLeft = false;
  bool m_bMoveUp = false;
  bool m_bMoveDown = false;
  bool m_bMoveForwardsInPlane = false;
  bool m_bMoveBackwardsInPlane = false;
  ezInt32 m_iDidMoveMouse[3] = {0, 0, 0}; // Left Click, Right Click, Middle Click

  bool m_bRotateLeft = false;
  bool m_bRotateRight = false;
  bool m_bRotateUp = false;
  bool m_bRotateDown = false;

  ezTime m_LastUpdate;
};

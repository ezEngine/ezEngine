#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>

class ezCamera;

/// \brief A simple orbit camera. Use LMB to rotate, wheel to zoom, Alt to slow down.
class EZ_EDITORFRAMEWORK_DLL ezOrbitCameraContext : public ezEditorInputContext
{
public:
  ezOrbitCameraContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView);

  void SetCamera(ezCamera* pCamera);
  ezCamera* GetCamera() const;

  void SetDefaultCameraRelative(const ezVec3& vDirection, float fDistanceScale);
  void SetDefaultCameraFixed(const ezVec3& vPosition);

  void MoveCameraToDefaultPosition();

  /// \brief Defines the box in which the user may move the camera around
  void SetOrbitVolume(const ezVec3& vCenterPos, const ezVec3& vHalfBoxSize);

  /// \brief The center point around which the camera can be moved and rotated.
  ezVec3 GetVolumeCenter() const { return m_Volume.GetCenter(); }

  /// \brief The half-size of the volume in which the camera may move around
  ezVec3 GetVolumeHalfSize() const { return m_Volume.GetHalfExtents(); }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoWheelEvent(QWheelEvent* e) override;
  virtual ezEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual ezEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override {}

private:
  virtual void UpdateContext() override;

  float GetCameraSpeed() const;

  void ResetCursor();
  void SetCurrentMouseMode();

  ezVec2I32 m_vLastMousePos;

  enum class Mode
  {
    Off,
    Orbit,
    Free,
    Pan,
  };

  Mode m_Mode = Mode::Off;
  ezCamera* m_pCamera;

  ezBoundingBox m_Volume;

  bool m_bFixedDefaultCamera = true;
  ezVec3 m_vDefaultCamera = ezVec3(1, 0, 0);

  bool m_bRun = false;
  bool m_bMoveForwards = false;
  bool m_bMoveBackwards = false;
  bool m_bMoveRight = false;
  bool m_bMoveLeft = false;
  bool m_bMoveUp = false;
  bool m_bMoveDown = false;

  ezTime m_LastUpdate;
};

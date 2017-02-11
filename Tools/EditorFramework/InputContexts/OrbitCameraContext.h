#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>

class ezCamera;

/// \brief A simple orbit camera. Use LMB to rotate, wheel to zoom, Alt to slow down.
class EZ_EDITORFRAMEWORK_DLL ezOrbitCameraContext : public ezEditorInputContext
{
public:
  ezOrbitCameraContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView);

  void SetCamera(ezCamera* pCamera);
  ezCamera* GetCamera() const;

  /// \brief Defines the box in which the user may move the camera around
  void SetOrbitVolume(const ezVec3& vCenterPos, const ezVec3& vHalfBoxSize, const ezVec3& vDefaultCameraPosition);

  /// \brief The center point around which the camera can be moved and rotated.
  ezVec3 GetVolumeCenter() const { return m_Volume.GetCenter(); }

  /// \brief The half-size of the volume in which the camera may move around
  ezVec3 GetVolumeHalfSize() const { return m_Volume.GetHalfExtents(); }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInut DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseMoveEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoWheelEvent(QWheelEvent* e) override;
  virtual ezEditorInut DoKeyPressEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override {}



private:
  virtual void UpdateContext() override {};

  void ResetCursor();
  void SetCurrentMouseMode();

  ezVec2I32 m_LastMousePos;

  enum class Mode
  {
    Off,
    Orbit,
    UpDown,
    MovePlane,
    Pan,
  };

  Mode m_Mode;
  ezCamera* m_pCamera;

  ezVec3 m_vDefaultCameraPosition;
  ezVec3 m_vOrbitPoint;

  ezBoundingBox m_Volume;
};

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
  void SetOrbitPoint(const ezVec3& vPos);
  const ezVec3 GetOrbitPoint() const;

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInut DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseMoveEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoWheelEvent(QWheelEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override {}

private:
  virtual void UpdateContext() override {};

  void ResetCursor();
  void SetCurrentMouseMode();

  ezVec2I32 m_LastMousePos;

  bool m_bOrbitCamera;

  ezCamera* m_pCamera;
  ezVec3 m_vOrbitPoint;

  bool m_bDidMoveMouse[3]; // Left Click, Right Click, Middle Click
};
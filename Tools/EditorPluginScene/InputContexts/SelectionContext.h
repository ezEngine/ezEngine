#pragma once

#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>

class QWidget;
class ezCamera;
struct ezObjectPickingResult;

class ezSelectionContext : public ezEditorInputContext
{
public:
  ezSelectionContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, const ezCamera* pCamera);

  void SetWindowConfig(const ezVec2I32& viewport)
  {
    m_Viewport = viewport;
  }

protected:
  virtual ezEditorInut doMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut doMouseReleaseEvent(QMouseEvent* e) override;

  virtual ezEditorInut doMouseMoveEvent(QMouseEvent* e) override;
  virtual ezEditorInut doKeyPressEvent(QKeyEvent* e) override;
  virtual ezEditorInut doKeyReleaseEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override {}

private:
  void OpenPickedMaterial(const ezObjectPickingResult& res) const;
  bool TryOpenMaterial(const ezString& sMatRef) const;

  bool m_bSelectOnMouseUp;
  const ezCamera* m_pCamera;
  ezVec2I32 m_Viewport;
};


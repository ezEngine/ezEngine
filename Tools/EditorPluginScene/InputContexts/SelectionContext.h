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

  virtual ezEditorInut mousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut mouseReleaseEvent(QMouseEvent* e) override;


  virtual ezEditorInut mouseMoveEvent(QMouseEvent* e) override;
  virtual ezEditorInut keyPressEvent(QKeyEvent* e) override;
  virtual ezEditorInut keyReleaseEvent(QKeyEvent* e) override;

protected:
  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override {}

private:
  void OpenPickedMaterial(const ezObjectPickingResult& res) const;
  bool TryOpenMaterial(const ezString& sMatRef) const;

  bool m_bSelectOnMouseUp;
  const ezCamera* m_pCamera;
  ezVec2I32 m_Viewport;
};


#pragma once

#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>

class QWidget;
class ezCamera;

class ezOrthoGizmoContext : public ezEditorInputContext
{
public:
  ezOrthoGizmoContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, const ezCamera* pCamera);

  void SetWindowConfig(const ezVec2I32& viewport)
  {
    m_Viewport = viewport;
  }

  virtual void FocusLost(bool bCancel);

  virtual ezEditorInut mousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut mouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInut mouseMoveEvent(QMouseEvent* e) override;

protected:
  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override {}

private:
  bool IsViewInOthoMode() const;

  bool m_bCanInteract;
  const ezCamera* m_pCamera;
  ezVec2I32 m_Viewport;
};


#pragma once

#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>

class QWidget;
class ezCamera;

class ezSelectionContext : public ezEditorInputContext
{
public:
  ezSelectionContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, const ezCamera* pCamera);

  void SetWindowConfig(const ezVec2I32& viewport)
  {
    m_Viewport = viewport;
  }

  virtual bool mousePressEvent(QMouseEvent* e) override;
  virtual bool mouseReleaseEvent(QMouseEvent* e) override;
  virtual bool mouseMoveEvent(QMouseEvent* e) override;
  virtual bool keyPressEvent(QKeyEvent* e) override;

protected:
  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override {}

private:
  bool m_bSelectOnMouseUp;
  const ezCamera* m_pCamera;
  ezVec2I32 m_Viewport;
};


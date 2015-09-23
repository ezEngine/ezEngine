#pragma once

#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>

class QWidget;
class ezCamera;

class ezSelectionContext : public ezEditorInputContext
{
public:
  ezSelectionContext(ezDocumentWindow3D* pOwnerWindow, ezEngineViewWidget* pOwnerView, const ezCamera* pCamera);

  void SetWindowConfig(const ezVec2I32& viewport)
  {
    m_Viewport = viewport;
  }

  virtual bool mousePressEvent(QMouseEvent* e) override;
  virtual bool mouseReleaseEvent(QMouseEvent* e) override;
  virtual bool mouseMoveEvent(QMouseEvent* e) override;
  virtual bool keyPressEvent(QKeyEvent* e) override;

protected:
  virtual void OnSetOwner(ezDocumentWindow3D* pOwnerWindow, ezEngineViewWidget* pOwnerView) override {}

private:
  const ezCamera* m_pCamera;
  ezVec2I32 m_Viewport;
};


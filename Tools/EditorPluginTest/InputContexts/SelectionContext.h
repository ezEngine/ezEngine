#pragma once

#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>

class QWidget;

class ezSelectionContext : public ezEditorInputContext
{
public:
  ezSelectionContext(ezDocumentBase* pDocument, ezDocumentWindow3D* pDocumentWindow);

  virtual bool mouseReleaseEvent(QMouseEvent* e) override;
  virtual bool mouseMoveEvent(QMouseEvent* e) override;

private:
  ezDocumentBase* m_pDocument;
  ezDocumentWindow3D* m_pDocumentWindow;
};


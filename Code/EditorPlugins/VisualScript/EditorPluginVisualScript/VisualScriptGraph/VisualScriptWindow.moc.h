#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtVisualScriptNodeScene;
class ezQtNodeView;

class ezQtVisualScriptWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtVisualScriptWindow(ezDocument* pDocument);
  ~ezQtVisualScriptWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "VisualScriptGraph"; }

private Q_SLOTS:

private:
  void SelectionEventHandler(const ezSelectionManagerEvent& e);

  ezQtVisualScriptNodeScene* m_pScene;
  ezQtNodeView* m_pView;
};

#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtVisualScriptNodeScene;
class ezQtVisualGraphView;

class ezQtVisualScriptWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtVisualScriptWindow(ezDocument* pDocument);
  ~ezQtVisualScriptWindow();

private Q_SLOTS:

private:
  void SelectionEventHandler(const ezSelectionManagerEvent& e);

  ezQtVisualScriptNodeScene* m_pScene;
  ezQtVisualGraphView* m_pView;
};

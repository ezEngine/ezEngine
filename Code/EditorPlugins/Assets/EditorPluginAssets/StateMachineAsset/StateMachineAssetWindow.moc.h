#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtStateMachineAssetScene;
class ezQtVisualGraphView;

class ezQtStateMachineAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtStateMachineAssetDocumentWindow(ezDocument* pDocument);
  ~ezQtStateMachineAssetDocumentWindow();

private Q_SLOTS:

private:
  ezQtStateMachineAssetScene* m_pScene;
  ezQtVisualGraphView* m_pView;
};

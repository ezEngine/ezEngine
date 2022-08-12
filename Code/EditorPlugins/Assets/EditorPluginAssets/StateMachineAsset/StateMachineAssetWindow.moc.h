#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtStateMachineAssetScene;
class ezQtNodeView;

class ezQtStateMachineAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtStateMachineAssetDocumentWindow(ezDocument* pDocument);
  ~ezQtStateMachineAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "StateMachineAsset"; }

private Q_SLOTS:

private:
  ezQtStateMachineAssetScene* m_pScene;
  ezQtNodeView* m_pView;
};

#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtAnimationGraphAssetScene;
class ezQtNodeView;

class ezQtAnimationGraphAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtAnimationGraphAssetDocumentWindow(ezDocument* pDocument);
  ~ezQtAnimationGraphAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "AnimationGraphAsset"; }

private Q_SLOTS:

private:
  void SelectionEventHandler(const ezSelectionManagerEvent& e);

  ezQtAnimationGraphAssetScene* m_pScene;
  ezQtNodeView* m_pView;
};

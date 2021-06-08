#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtAnimationControllerAssetScene;
class ezQtNodeView;

class ezQtAnimationControllerAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtAnimationControllerAssetDocumentWindow(ezDocument* pDocument);
  ~ezQtAnimationControllerAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "AnimationControllerAsset"; }

private Q_SLOTS:

private:
  ezQtAnimationControllerAssetScene* m_pScene;
  ezQtNodeView* m_pView;
};

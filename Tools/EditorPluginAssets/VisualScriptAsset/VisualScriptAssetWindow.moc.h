#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtVisualScriptAssetScene;
class ezQtNodeView;

class ezQtVisualScriptAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtVisualScriptAssetDocumentWindow(ezDocument* pDocument);
  ~ezQtVisualScriptAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const { return "VisualScriptAsset"; }

private slots:

private:
  ezQtVisualScriptAssetScene* m_pScene;
  ezQtNodeView* m_pView;
};

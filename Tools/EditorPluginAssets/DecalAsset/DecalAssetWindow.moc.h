#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>

class ezDecalAssetDocument;
//class ezQtDecalViewWidget;

class ezQtDecalAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtDecalAssetDocumentWindow(ezDecalAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const { return "DecalAsset"; }

private:
  virtual void InternalRedraw() override;
  void SendRedrawMsg();

  //ezSceneViewConfig m_ViewConfig;
  //ezQtDecalViewWidget* m_pViewWidget;
};


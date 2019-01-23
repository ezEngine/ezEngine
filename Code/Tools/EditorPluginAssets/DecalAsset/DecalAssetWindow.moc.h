#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>

class ezDecalAssetDocument;
class ezQtOrbitCamViewWidget;

class ezQtDecalAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtDecalAssetDocumentWindow(ezDecalAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const { return "DecalAsset"; }

private:
  virtual void InternalRedraw() override;
  void SendRedrawMsg();

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget;
};


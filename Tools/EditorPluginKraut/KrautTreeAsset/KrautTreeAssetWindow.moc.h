#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtOrbitCamViewWidget;

class ezQtKrautTreeAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtKrautTreeAssetDocumentWindow(ezAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const { return "KrautTreeAsset"; }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;

private:
  void SendRedrawMsg();
  void QueryObjectBBox(ezInt32 iPurpose);

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget;
  ezKrautTreeAssetDocument* m_pAssetDoc;
};

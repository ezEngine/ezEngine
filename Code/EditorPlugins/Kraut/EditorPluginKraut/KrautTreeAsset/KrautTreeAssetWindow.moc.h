#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtOrbitCamViewWidget;
class ezQtPropertyGridWidget;

class ezQtKrautTreeAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtKrautTreeAssetDocumentWindow(ezAssetDocument* pDocument);
  ~ezQtKrautTreeAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "KrautTreeAsset"; }

  ezKrautTreeAssetDocument* GetKrautDocument() const
  {
    return static_cast<ezKrautTreeAssetDocument*>(GetDocument());
  }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

private Q_SLOTS:
  void onBranchTypeSelected(int index);

private:
  void SendRedrawMsg();
  void QueryObjectBBox(ezInt32 iPurpose);
  void UpdatePreview();
  void RestoreResource();

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget = nullptr;
  ezKrautTreeAssetDocument* m_pAssetDoc = nullptr;
  ezQtPropertyGridWidget* m_pBranchProps = nullptr;
};

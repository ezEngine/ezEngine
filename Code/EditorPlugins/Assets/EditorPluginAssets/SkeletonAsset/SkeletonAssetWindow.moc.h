#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <Foundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtOrbitCamViewWidget;
class ezSelectionContext;

class ezQtSkeletonAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtSkeletonAssetDocumentWindow(ezSkeletonAssetDocument* pDocument);
  ~ezQtSkeletonAssetDocumentWindow();

  ezSkeletonAssetDocument* GetSkeletonDocument();
  virtual const char* GetWindowLayoutGroupName() const override { return "SkeletonAsset"; }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;

private:
  void SendRedrawMsg();
  void QueryObjectBBox(ezInt32 iPurpose = 0);
  void SelectionEventHandler(const ezSelectionManagerEvent& e);
  void SkeletonAssetEventHandler(const ezSkeletonAssetEvent& e);

  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void CommandEventHandler(const ezCommandHistoryEvent&);

  void SendLiveResourcePreview();
  void RestoreResource();

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget = nullptr;
};

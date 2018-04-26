#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>

class QLabel;
class QScrollArea;
class ezQtImageWidget;
class ezQtSkeletonViewWidget;

class ezQtSkeletonAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtSkeletonAssetDocumentWindow(ezSkeletonAssetDocument* pDocument);

  ezSkeletonAssetDocument* GetSkeletonDocument();
  virtual const char* GetWindowLayoutGroupName() const { return "SkeletonAsset"; }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;

private:
  void SendRedrawMsg();
  void QueryObjectBBox(ezInt32 iPurpose);

  ezEngineViewConfig m_ViewConfig;
  ezQtSkeletonViewWidget* m_pViewWidget;
};

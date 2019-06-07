#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtOrbitCamViewWidget;

class ezQtAnimationClipAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtAnimationClipAssetDocumentWindow(ezAnimationClipAssetDocument* pDocument);

  ezAnimationClipAssetDocument* GetAnimationClipDocument();
  virtual const char* GetWindowLayoutGroupName() const { return "AnimationClipAsset"; }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;

private:
  void SendRedrawMsg();
  void QueryObjectBBox(ezInt32 iPurpose);

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget;
};

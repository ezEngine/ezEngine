#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <Foundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtOrbitCamViewWidget;

class ezQtAnimatedMeshAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtAnimatedMeshAssetDocumentWindow(ezAnimatedMeshAssetDocument* pDocument);

  ezAnimatedMeshAssetDocument* GetMeshDocument();
  virtual const char* GetWindowLayoutGroupName() const { return "AnimatedMeshAsset"; }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;

private:
  void SendRedrawMsg();
  void QueryObjectBBox(ezInt32 iPurpose);

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget;
};

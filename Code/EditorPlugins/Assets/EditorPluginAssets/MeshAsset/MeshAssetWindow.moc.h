#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>

class ezQtOrbitCamViewWidget;

class ezQtMeshAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtMeshAssetDocumentWindow(ezMeshAssetDocument* pDocument);

  ezMeshAssetDocument* GetMeshDocument();
  virtual const char* GetWindowLayoutGroupName() const { return "MeshAsset"; }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;

private:
  void SendRedrawMsg();
  void QueryObjectBBox(ezInt32 iPurpose);

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget;
};

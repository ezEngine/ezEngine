#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtOrbitCamViewWidget;

class ezQtJoltCollisionMeshAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtJoltCollisionMeshAssetDocumentWindow(ezAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "JoltCollisionMeshAsset"; }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;

private:
  void SendRedrawMsg();
  void QueryObjectBBox(ezInt32 iPurpose);

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget;
  ezAssetDocument* m_pAssetDoc;
};

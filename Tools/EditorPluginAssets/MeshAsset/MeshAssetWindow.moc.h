#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>

class QLabel;
class QScrollArea;
class ezQtImageWidget;
class ezQtMeshViewWidget;

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
  void QueryObjectBBox();

  ezSceneViewConfig m_ViewConfig;
  ezQtMeshViewWidget* m_pViewWidget;

  QLabel* m_pLabelInfo;
};

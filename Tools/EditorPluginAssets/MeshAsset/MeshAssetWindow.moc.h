#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>

class QLabel;
class QScrollArea;
class QtImageWidget;
class ezQtMeshViewWidget;

class ezMeshAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezMeshAssetDocumentWindow(ezMeshAssetDocument* pDocument);
  ~ezMeshAssetDocumentWindow();

  ezMeshAssetDocument* GetMeshDocument();
  virtual const char* GetWindowLayoutGroupName() const { return "MeshAsset"; }

protected:
  virtual void InternalRedraw() override;

private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void MeshAssetDocumentEventHandler(const ezAssetDocument::AssetEvent& e);
  void SendRedrawMsg();

  ezSceneViewConfig m_ViewConfig;
  ezQtMeshViewWidget* m_pViewWidget;

  QLabel* m_pLabelInfo;
};
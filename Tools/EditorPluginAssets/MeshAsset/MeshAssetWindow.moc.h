#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>

class QLabel;
class QScrollArea;
class QtImageWidget;

class ezMeshAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezMeshAssetDocumentWindow(ezDocument* pDocument);
  ~ezMeshAssetDocumentWindow();

  virtual const char* GetGroupName() const { return "MeshAsset"; }

private slots:
  

private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void MeshAssetDocumentEventHandler(const ezAssetDocument::AssetEvent& e);

  ezMeshAssetDocument* m_pAssetDoc;
  QLabel* m_pLabelInfo;
};
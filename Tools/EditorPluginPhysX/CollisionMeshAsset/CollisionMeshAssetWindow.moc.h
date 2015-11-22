#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAsset.h>

class QLabel;
class QScrollArea;
class QtImageWidget;

class ezCollisionMeshAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezCollisionMeshAssetDocumentWindow(ezDocument* pDocument);
  ~ezCollisionMeshAssetDocumentWindow();

  virtual const char* GetGroupName() const { return "CollisionMeshAsset"; }

private slots:
  

private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void MeshAssetDocumentEventHandler(const ezAssetDocument::AssetEvent& e);

  ezCollisionMeshAssetDocument* m_pAssetDoc;
  QLabel* m_pLabelInfo;
};
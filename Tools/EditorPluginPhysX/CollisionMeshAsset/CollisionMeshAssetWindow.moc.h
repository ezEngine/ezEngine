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

  virtual const char* GetWindowLayoutGroupName() const { return "CollisionMeshAsset"; }

private:
  ezCollisionMeshAssetDocument* m_pAssetDoc;
  QLabel* m_pLabelInfo;
};
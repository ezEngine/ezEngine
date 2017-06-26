#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAsset.h>
#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>

class QLabel;
class QScrollArea;
class ezQtImageWidget;
class ezQtCollisionMeshViewWidget;

class ezQtCollisionMeshAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtCollisionMeshAssetDocumentWindow(ezAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const { return "CollisionMeshAsset"; }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;

private:
  void SendRedrawMsg();
  void QueryObjectBBox();

  ezSceneViewConfig m_ViewConfig;
  ezQtCollisionMeshViewWidget* m_pViewWidget;
  ezCollisionMeshAssetDocument* m_pAssetDoc;
};
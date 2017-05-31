#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

class ezQtCollisionMeshAssetDocumentWindow;
class ezOrbitCameraContext;

class ezQtCollisionMeshViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtCollisionMeshViewWidget(QWidget* pParent, ezQtCollisionMeshAssetDocumentWindow* pOwnerWindow, ezSceneViewConfig* pViewConfig);
  ~ezQtCollisionMeshViewWidget();

  ezOrbitCameraContext* GetOrbitCamera() { return m_pOrbitCameraContext; }

private:
  ezOrbitCameraContext* m_pOrbitCameraContext;
};

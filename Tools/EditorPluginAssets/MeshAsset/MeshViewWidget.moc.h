#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

class ezQtMeshAssetDocumentWindow;
class ezOrbitCameraContext;

class ezQtMeshViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtMeshViewWidget(QWidget* pParent, ezQtMeshAssetDocumentWindow* pOwnerWindow, ezSceneViewConfig* pViewConfig);
  ~ezQtMeshViewWidget();

  ezOrbitCameraContext* GetOrbitCamera() { return m_pOrbitCameraContext; }

private:
  ezOrbitCameraContext* m_pOrbitCameraContext;
};

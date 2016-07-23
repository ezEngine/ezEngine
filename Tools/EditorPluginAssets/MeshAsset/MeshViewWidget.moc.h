#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

class ezMeshAssetDocumentWindow;
class ezOrbitCameraContext;

class ezQtMeshViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtMeshViewWidget(QWidget* pParent, ezMeshAssetDocumentWindow* pOwnerWindow, ezSceneViewConfig* pViewConfig);
  ~ezQtMeshViewWidget();

private:
  ezOrbitCameraContext* m_pOrbitCameraContext;
};

#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

class ezMaterialAssetDocumentWindow;
class ezOrbitCameraContext;

class ezQtMaterialViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtMaterialViewWidget(QWidget* pParent, ezMaterialAssetDocumentWindow* pOwnerWindow, ezSceneViewConfig* pViewConfig);
  ~ezQtMaterialViewWidget();

private:
  ezOrbitCameraContext* m_pOrbitCameraContext;
};

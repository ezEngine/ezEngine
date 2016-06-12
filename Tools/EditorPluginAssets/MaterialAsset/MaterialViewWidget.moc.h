#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>

class ezMaterialAssetDocumentWindow;

class ezQtMaterialViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtMaterialViewWidget(QWidget* pParent, ezMaterialAssetDocumentWindow* pOwnerWindow, ezSceneViewConfig* pViewConfig);
  ~ezQtMaterialViewWidget();
};

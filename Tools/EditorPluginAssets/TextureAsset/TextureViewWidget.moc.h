#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

class ezQtTextureAssetDocumentWindow;
class ezOrbitCameraContext;

class ezQtTextureViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtTextureViewWidget(QWidget* pParent, ezQtTextureAssetDocumentWindow* pOwnerWindow, ezSceneViewConfig* pViewConfig);
  ~ezQtTextureViewWidget();

private:
  ezOrbitCameraContext* m_pOrbitCameraContext;
};

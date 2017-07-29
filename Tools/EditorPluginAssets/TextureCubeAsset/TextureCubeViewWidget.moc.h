#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <Foundation/Types/UniquePtr.h>

class ezQtTextureCubeAssetDocumentWindow;
class ezOrbitCameraContext;

class ezQtTextureCubeViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtTextureCubeViewWidget(QWidget* pParent, ezQtTextureCubeAssetDocumentWindow* pOwnerWindow, ezSceneViewConfig* pViewConfig);
  ~ezQtTextureCubeViewWidget();

private:
  ezUniquePtr<ezOrbitCameraContext> m_pOrbitCameraContext;
};

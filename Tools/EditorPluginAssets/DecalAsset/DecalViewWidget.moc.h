#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <Foundation/Types/UniquePtr.h>

class ezQtDecalAssetDocumentWindow;
class ezOrbitCameraContext;

class ezQtDecalViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtDecalViewWidget(QWidget* pParent, ezQtDecalAssetDocumentWindow* pOwnerWindow, ezSceneViewConfig* pViewConfig);
  ~ezQtDecalViewWidget();

private:
  ezUniquePtr<ezOrbitCameraContext> m_pOrbitCameraContext;
};

#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <Foundation/Types/UniquePtr.h>

class ezQtTextureAssetDocumentWindow;
class ezOrbitCameraContext;

class ezQtTextureViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtTextureViewWidget(QWidget* pParent, ezQtTextureAssetDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig);
  ~ezQtTextureViewWidget();

private:
  ezUniquePtr<ezOrbitCameraContext> m_pOrbitCameraContext;
};

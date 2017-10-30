#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <Foundation/Types/UniquePtr.h>

class ezQtMeshAssetDocumentWindow;
class ezOrbitCameraContext;

class ezQtMeshViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtMeshViewWidget(QWidget* pParent, ezQtMeshAssetDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig);
  ~ezQtMeshViewWidget();

  ezOrbitCameraContext* GetOrbitCamera() { return m_pOrbitCameraContext.Borrow(); }

private:
  ezUniquePtr<ezOrbitCameraContext> m_pOrbitCameraContext;
};

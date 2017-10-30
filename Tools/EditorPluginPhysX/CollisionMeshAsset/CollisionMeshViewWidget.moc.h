#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <Foundation/Types/UniquePtr.h>

class ezQtCollisionMeshAssetDocumentWindow;
class ezOrbitCameraContext;

class ezQtCollisionMeshViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtCollisionMeshViewWidget(QWidget* pParent, ezQtCollisionMeshAssetDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig);
  ~ezQtCollisionMeshViewWidget();

  ezOrbitCameraContext* GetOrbitCamera() { return m_pOrbitCameraContext.Borrow(); }

private:
  ezUniquePtr<ezOrbitCameraContext> m_pOrbitCameraContext;
};

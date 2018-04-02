#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <Foundation/Types/UniquePtr.h>

class ezQtSkeletonAssetDocumentWindow;
class ezOrbitCameraContext;

class ezQtSkeletonViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtSkeletonViewWidget(QWidget* pParent, ezQtSkeletonAssetDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig);
  ~ezQtSkeletonViewWidget();

  ezOrbitCameraContext* GetOrbitCamera() { return m_pOrbitCameraContext.Borrow(); }

private:
  ezUniquePtr<ezOrbitCameraContext> m_pOrbitCameraContext;
};

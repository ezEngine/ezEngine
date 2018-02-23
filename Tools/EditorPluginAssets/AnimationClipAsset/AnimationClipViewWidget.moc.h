#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <Foundation/Types/UniquePtr.h>

class ezQtAnimationClipAssetDocumentWindow;
class ezOrbitCameraContext;

class ezQtAnimationClipViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtAnimationClipViewWidget(QWidget* pParent, ezQtAnimationClipAssetDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig);
  ~ezQtAnimationClipViewWidget();

  ezOrbitCameraContext* GetOrbitCamera() { return m_pOrbitCameraContext.Borrow(); }

private:
  ezUniquePtr<ezOrbitCameraContext> m_pOrbitCameraContext;
};

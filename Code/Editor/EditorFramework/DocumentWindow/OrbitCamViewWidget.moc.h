#pragma once

#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <Foundation/Basics.h>
#include <Foundation/Types/UniquePtr.h>

class ezOrbitCameraContext;
class ezSelectionContext;

class EZ_EDITORFRAMEWORK_DLL ezQtOrbitCamViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtOrbitCamViewWidget(ezQtEngineDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig, bool bPicking = false);
  ~ezQtOrbitCamViewWidget();

  void ConfigureOrbitCameraVolume(const ezVec3& vCenterPos, const ezVec3& vHalfBoxSize, const ezVec3& vDefaultCameraPosition);

  ezOrbitCameraContext* GetOrbitCamera();


  virtual void SyncToEngine() override;

private:
  ezUniquePtr<ezOrbitCameraContext> m_pOrbitCameraContext;
  ezUniquePtr<ezSelectionContext> m_pSelectionContext;
};

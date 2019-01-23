#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <Foundation/Types/UniquePtr.h>

class ezOrbitCameraContext;

class EZ_EDITORFRAMEWORK_DLL ezQtOrbitCamViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:

  ezQtOrbitCamViewWidget(ezQtEngineDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig);
  ~ezQtOrbitCamViewWidget();

  void ConfigureOrbitCameraVolume(const ezVec3& vCenterPos, const ezVec3& vHalfBoxSize, const ezVec3& vDefaultCameraPosition);

  ezOrbitCameraContext* GetOrbitCamera();

private:
  ezUniquePtr<ezOrbitCameraContext> m_pOrbitCameraContext;
};

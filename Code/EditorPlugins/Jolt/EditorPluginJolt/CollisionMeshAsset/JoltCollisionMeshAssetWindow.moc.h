#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAsset.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtOrbitCamViewWidget;

class ezQtJoltCollisionMeshAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtJoltCollisionMeshAssetDocumentWindow(ezAssetDocument* pDocument);

  virtual int GetCameraMode() const override { return m_iCameraMode; }
  virtual void SetCameraMode(int iMode) override;

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;

private:
  void SendRedrawMsg();
  void QueryObjectBBox(ezInt32 iPurpose = 0);

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget;
  ezUniquePtr<ezCameraMoveContext> m_pCameraFlyContext;
  int m_iCameraMode = 0;
  ezAssetDocument* m_pAssetDoc;
};

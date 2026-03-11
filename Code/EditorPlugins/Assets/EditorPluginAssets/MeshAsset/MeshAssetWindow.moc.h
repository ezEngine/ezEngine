#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/MeshAsset/MeshEditorContext.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

#include <QPointer>

class ezQtOrbitCamViewWidget;

class ezQtMeshAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtMeshAssetDocumentWindow(ezMeshAssetDocument* pDocument);
  ~ezQtMeshAssetDocumentWindow();

  ezMeshAssetDocument* GetMeshDocument();

  virtual int GetCameraMode() const override { return m_iCameraMode; }
  virtual void SetCameraMode(int iMode) override;

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;

protected Q_SLOTS:
  void HighlightTimer();

private:
  void SendRedrawMsg();
  void QueryObjectBBox(ezInt32 iPurpose = 0);
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  bool UpdatePreview();

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget;
  ezUniquePtr<ezMeshEditorInputContext> m_pMeshEditorInputContext;
  ezUniquePtr<ezCameraMoveContext> m_pCameraFlyContext;
  int m_iCameraMode = 0;
  ezUInt32 m_uiHighlightSlots = 0;
  QPointer<QTimer> m_pHighlightTimer;
};

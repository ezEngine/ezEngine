#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

#include <QPointer>
#include <QTimer>

class ezQtOrbitCamViewWidget;

class ezQtMeshAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtMeshAssetDocumentWindow(ezMeshAssetDocument* pDocument);
  ~ezQtMeshAssetDocumentWindow();

  ezMeshAssetDocument* GetMeshDocument();
  virtual const char* GetWindowLayoutGroupName() const override { return "MeshAsset"; }

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
  ezUInt32 m_uiHighlightSlots = 0;
  QPointer<QTimer> m_pHighlightTimer;
};

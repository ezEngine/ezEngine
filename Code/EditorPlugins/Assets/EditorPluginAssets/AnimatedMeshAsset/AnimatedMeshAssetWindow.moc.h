#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <Foundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

#include <QPointer>

class ezQtOrbitCamViewWidget;

class ezQtAnimatedMeshAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtAnimatedMeshAssetDocumentWindow(ezAnimatedMeshAssetDocument* pDocument);
  ~ezQtAnimatedMeshAssetDocumentWindow();

  ezAnimatedMeshAssetDocument* GetMeshDocument();
  virtual const char* GetWindowLayoutGroupName() const override { return "AnimatedMeshAsset"; }

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

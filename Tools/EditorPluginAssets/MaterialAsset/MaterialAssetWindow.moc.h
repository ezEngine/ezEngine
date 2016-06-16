#pragma once

#include <Foundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>

class QLabel;
class QScrollArea;
class QtImageWidget;
class ezMaterialAssetDocument;
class ezQtMaterialViewWidget;

class ezMaterialAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezMaterialAssetDocumentWindow(ezMaterialAssetDocument* pDocument);
  ~ezMaterialAssetDocumentWindow();

  ezMaterialAssetDocument* GetMaterialDocument();
  virtual const char* GetGroupName() const { return "MaterialAsset"; }

protected:
  virtual void InternalRedraw() override;

private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void SendRedrawMsg();
  void RestoreResource();

  ezSceneViewConfig m_ViewConfig;
  ezQtMaterialViewWidget* m_pViewWidget;
};
#pragma once

#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezProcGenGraphAssetDocument;

class ezQtNodeScene;
class ezQtNodeView;

class ezProcGenGraphAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezProcGenGraphAssetDocumentWindow(ezProcGenGraphAssetDocument* pDocument);
  ~ezProcGenGraphAssetDocumentWindow();

  ezProcGenGraphAssetDocument* GetProcGenGraphDocument();

  virtual const char* GetWindowLayoutGroupName() const override { return "ProcGenAsset"; }

private Q_SLOTS:


private:
  void UpdatePreview();
  void RestoreResource();

  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

  ezQtNodeScene* m_pScene;
  ezQtNodeView* m_pView;
};


#pragma once

#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezProcGenGraphAssetDocument;

class ezQtNodeScene;
class ezQtNodeView;
struct ezCommandHistoryEvent;

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

  // needed for setting the debug pin
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void TransationEventHandler(const ezCommandHistoryEvent& e);

  ezQtNodeScene* m_pScene;
  ezQtNodeView* m_pView;
};

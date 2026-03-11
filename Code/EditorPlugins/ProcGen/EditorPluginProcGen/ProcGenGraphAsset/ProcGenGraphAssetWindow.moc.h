#pragma once

#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezProcGenGraphAssetDocument;

class ezQtVisualGraphScene;
class ezQtVisualGraphView;
struct ezCommandHistoryEvent;

class ezProcGenGraphAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezProcGenGraphAssetDocumentWindow(ezProcGenGraphAssetDocument* pDocument);
  ~ezProcGenGraphAssetDocumentWindow();

  ezProcGenGraphAssetDocument* GetProcGenGraphDocument();

private Q_SLOTS:


private:
  void UpdatePreview();
  void RestoreResource();

  // needed for setting the debug pin
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void TransactionEventHandler(const ezCommandHistoryEvent& e);

  void SelectionEventHandler(const ezSelectionManagerEvent& e);

  ezQtVisualGraphScene* m_pScene;
  ezQtVisualGraphView* m_pView;
};

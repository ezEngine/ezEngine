#pragma once

#include <EditorFramework/Assets/AssetCheckRule.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetCheckPanel.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <ToolsFoundation/Document/DocumentManager.h>

class QTreeWidgetItem;
namespace ads
{
  class CDockManager;
}

/// Application wide panel that runs asset check rules over a selection of assets and lists the reported issues.
class EZ_EDITORFRAMEWORK_DLL ezQtAssetCheckPanel : public ezQtApplicationPanel, public Ui_AssetCheckPanel
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtAssetCheckPanel);

public:
  ezQtAssetCheckPanel(ads::CDockManager* pDockManager);
  ~ezQtAssetCheckPanel();

  void FillRuleList();

protected:
  virtual bool eventFilter(QObject* pWatched, QEvent* pEvent) override;

private:
  void RunButtonClicked();
  void ResultTreeItemDoubleClicked(QTreeWidgetItem* pItem, int iColumn);
  
  void UpdateAssetTypeCombo();
  void DocumentManagerEventHandler(const ezDocumentManager::Event& e);

  ezDynamicArray<ezAssetCheckRule*> m_Rules;
};

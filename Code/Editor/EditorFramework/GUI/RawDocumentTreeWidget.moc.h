#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <GuiFoundation/Widgets/ItemView.moc.h>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <memory>

class ezQtTreeSearchFilterModel;
class ezSelectionManager;

class EZ_EDITORFRAMEWORK_DLL ezQtDocumentTreeView : public ezQtItemView<QTreeView>
{
  Q_OBJECT

public:
  ezQtDocumentTreeView(QWidget* pParent);
  ezQtDocumentTreeView(QWidget* pParent, ezDocument* pDocument, std::unique_ptr<ezQtDocumentTreeModel> pCustomModel, ezSelectionManager* pSelection = nullptr);
  ~ezQtDocumentTreeView();

  void Initialize(ezDocument* pDocument, std::unique_ptr<ezQtDocumentTreeModel> pCustomModel, ezSelectionManager* pSelection = nullptr);

  void EnsureLastSelectedItemVisible();

  void SetAllowDragDrop(bool bAllow);
  void SetAllowDeleteObjects(bool bAllow);

  ezQtTreeSearchFilterModel* GetProxyFilterModel() const { return m_pFilterModel.get(); }

protected:
  virtual bool event(QEvent* pEvent) override;

private Q_SLOTS:
  void on_selectionChanged_triggered(const QItemSelection& selected, const QItemSelection& deselected);

private:
  void SelectionEventHandler(const ezSelectionManagerEvent& e);

private:
  std::unique_ptr<ezQtDocumentTreeModel> m_pModel;
  std::unique_ptr<ezQtTreeSearchFilterModel> m_pFilterModel;
  ezSelectionManager* m_pSelectionManager = nullptr;
  ezDocument* m_pDocument = nullptr;
  bool m_bBlockSelectionSignal = false;
  bool m_bAllowDeleteObjects = false;
};

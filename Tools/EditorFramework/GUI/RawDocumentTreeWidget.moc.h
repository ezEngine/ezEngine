#pragma once

#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <QTreeView>
#include <memory>
#include <QSortFilterProxyModel>

class EZ_EDITORFRAMEWORK_DLL ezQtScenegraphFilterModel : public QSortFilterProxyModel
{
  Q_OBJECT

public:
  ezQtScenegraphFilterModel(QWidget* parent) : QSortFilterProxyModel(parent)
  {
  }

  void SetFilterText(const QString& text);

protected:
  void RecomputeVisibleItems();
  bool UpdateVisibility(const QModelIndex& idx);
  virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

  QAbstractItemModel* m_pSourceModel;
  QString m_sFilterText;
  ezMap<QModelIndex, bool> m_Visible;
};

class EZ_EDITORFRAMEWORK_DLL ezQtDocumentTreeView : public QTreeView
{
  Q_OBJECT

public:

  ezQtDocumentTreeView(QWidget* pParent);
  ezQtDocumentTreeView(QWidget* pParent, ezDocument* pDocument, const ezRTTI* pBaseClass, const char* szChildProperty, std::unique_ptr<ezQtDocumentTreeModel> pCustomModel = nullptr);
  ~ezQtDocumentTreeView();

  void Initialize(ezDocument* pDocument, const ezRTTI* pBaseClass, const char* szChildProperty, std::unique_ptr<ezQtDocumentTreeModel> pCustomModel = nullptr);

  void EnsureLastSelectedItemVisible();

  void SetAllowDragDrop(bool bAllow);

  ezQtScenegraphFilterModel* GetProxyFilterModel() const { return m_pFilterModel.get(); }

protected:
  virtual void keyPressEvent(QKeyEvent* e) override;

private slots:
  void on_selectionChanged_triggered(const QItemSelection& selected, const QItemSelection& deselected);

private:
  void SelectionEventHandler(const ezSelectionManagerEvent& e);

private:
  std::unique_ptr<ezQtDocumentTreeModel> m_pModel;
  std::unique_ptr<ezQtScenegraphFilterModel> m_pFilterModel;
  ezDocument* m_pDocument;
  bool m_bBlockSelectionSignal;
};


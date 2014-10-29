#include <PCH.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <ToolsFoundation/Document/Document.h>


ezRawDocumentTreeWidget::ezRawDocumentTreeWidget(QWidget* pParent, const ezDocumentBase* pDocument) : 
  QTreeView(pParent),
  m_Model(pDocument->GetObjectTree()),
  m_pDocument(pDocument)
{
  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  setModel(&m_Model);
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);

  EZ_VERIFY(connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(on_selectionChanged_triggered(const QItemSelection&, const QItemSelection&))) != nullptr, "signal/slot connection failed");

  QModelIndexList selection = selectionModel()->selectedIndexes();
}

void ezRawDocumentTreeWidget::on_selectionChanged_triggered(const QItemSelection& selected, const QItemSelection& deselected)
{
  QModelIndexList selection = selectionModel()->selectedIndexes();

  ezDeque<const ezDocumentObjectBase*> sel;

  foreach(QModelIndex index, selection)
  {
    if (index.isValid())
      sel.PushBack((const ezDocumentObjectBase*) index.internalPointer());
  }

  // TODO const cast
  ((ezSelectionManager*) m_pDocument->GetSelectionManager())->SetSelection(sel);
}

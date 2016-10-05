#include <PCH.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <QKeyEvent>
#include <QSortFilterProxyModel>

void ezQtScenegraphFilterModel::SetFilterText(const QString& text)
{
  // only clear the current visible state, if the new filter text got shorter
  // this way previous information stays valid and can be used to early out
  if (!text.contains(m_sFilterText, Qt::CaseInsensitive))
    m_Visible.Clear();

  m_sFilterText = text;

  if (!m_sFilterText.isEmpty())
  {
    RecomputeVisibleItems();
  }

  invalidateFilter();
}

void ezQtScenegraphFilterModel::RecomputeVisibleItems()
{
  m_pSourceModel = sourceModel();

  const int numRows = m_pSourceModel->rowCount();

  for (int r = 0; r < numRows; ++r)
  {
    QModelIndex idx = m_pSourceModel->index(r, 0);

    UpdateVisibility(idx);
  }
}

bool ezQtScenegraphFilterModel::UpdateVisibility(const QModelIndex& idx)
{
  bool bExisted = false;
  auto itVis = m_Visible.FindOrAdd(idx, &bExisted);

  // early out, if we already know that this object is invisible (from previous searches)
  if (bExisted && !itVis.Value())
    return false;

  QString name = m_pSourceModel->data(idx, Qt::DisplayRole).toString();

  bool bAnyVisible = false;

  if (name.contains(m_sFilterText, Qt::CaseInsensitive))
    bAnyVisible = true;

  const int numRows = m_pSourceModel->rowCount(idx);

  for (int r = 0; r < numRows; ++r)
  {
    QModelIndex idxChild = m_pSourceModel->index(r, 0, idx);

    if (UpdateVisibility(idxChild))
      bAnyVisible = true;
  }

  itVis.Value() = bAnyVisible;
  return bAnyVisible;
}

bool ezQtScenegraphFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
  QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);

  if (m_sFilterText.isEmpty())
    return true;

  auto itVis = m_Visible.Find(idx);

  return itVis.IsValid() && itVis.Value();
}

ezQtDocumentTreeView::ezQtDocumentTreeView(QWidget* parent)
  : QTreeView(parent)
{

}

ezQtDocumentTreeView::ezQtDocumentTreeView(QWidget* pParent, ezDocument* pDocument, const ezRTTI* pBaseClass, const char* szChildProperty, std::unique_ptr<ezQtDocumentTreeModel> pCustomModel)
  : QTreeView(pParent)
{
  Initialize(pDocument, pBaseClass, szChildProperty, std::move(pCustomModel));
}

void ezQtDocumentTreeView::Initialize(ezDocument* pDocument, const ezRTTI* pBaseClass, const char* szChildProperty, std::unique_ptr<ezQtDocumentTreeModel> pCustomModel)
{
  m_pDocument = pDocument;

  if (pCustomModel)
    m_pModel = std::move(pCustomModel);
  else
    m_pModel.reset(new ezQtDocumentTreeModel(pDocument->GetObjectManager(), pBaseClass, szChildProperty));

  m_pFilterModel.reset(new ezQtScenegraphFilterModel(this));
  m_pFilterModel->setSourceModel(m_pModel.get());

  m_bBlockSelectionSignal = false;
  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  setModel(m_pFilterModel.get());
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);
  setHeaderHidden(true);
  setExpandsOnDoubleClick(true);
  setEditTriggers(QAbstractItemView::EditTrigger::EditKeyPressed);

  EZ_VERIFY(connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(on_selectionChanged_triggered(const QItemSelection&, const QItemSelection&))) != nullptr, "signal/slot connection failed");
  pDocument->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtDocumentTreeView::SelectionEventHandler, this));
}

ezQtDocumentTreeView::~ezQtDocumentTreeView()
{
  m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtDocumentTreeView::SelectionEventHandler, this));
}

void ezQtDocumentTreeView::on_selectionChanged_triggered(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (m_bBlockSelectionSignal)
    return;

  QModelIndexList selection = selectionModel()->selectedIndexes();

  ezDeque<const ezDocumentObject*> sel;

  foreach(QModelIndex index, selection)
  {
    if (index.isValid())
    {
      index = m_pFilterModel->mapToSource(index);

      if (index.isValid())
        sel.PushBack((const ezDocumentObject*)index.internalPointer());
    }
  }

  // TODO const cast
  ((ezSelectionManager*) m_pDocument->GetSelectionManager())->SetSelection(sel);
}

void ezQtDocumentTreeView::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  switch (e.m_Type)
  {
  case ezSelectionManagerEvent::Type::SelectionCleared:
    {
      // Can't block signals on selection model or view won't update.
      m_bBlockSelectionSignal = true;
      selectionModel()->clear();
      m_bBlockSelectionSignal = false;
    }
    break;
  case ezSelectionManagerEvent::Type::SelectionSet:
  case ezSelectionManagerEvent::Type::ObjectAdded:
  case ezSelectionManagerEvent::Type::ObjectRemoved:
    {
      // Can't block signals on selection model or view won't update.
      m_bBlockSelectionSignal = true;
      QItemSelection selection;
      
      for (const ezDocumentObject* pObject : m_pDocument->GetSelectionManager()->GetSelection())
      {
        auto index = m_pModel->ComputeModelIndex(pObject);
        index = m_pFilterModel->mapFromSource(index);

        if (index.isValid())
          selection.select(index, index);
      }
      selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows | QItemSelectionModel::NoUpdate);
      m_bBlockSelectionSignal = false;
    }
    break;
  }
}

void ezQtDocumentTreeView::EnsureLastSelectedItemVisible()
{
  if (m_pDocument->GetSelectionManager()->GetSelection().IsEmpty())
    return;

  const ezDocumentObject* pObject = m_pDocument->GetSelectionManager()->GetSelection().PeekBack();

  auto index = m_pModel->ComputeModelIndex(pObject);
  index = m_pFilterModel->mapFromSource(index);

  if (index.isValid())
    scrollTo(index, QAbstractItemView::EnsureVisible);
}


void ezQtDocumentTreeView::SetAllowDragDrop(bool bAllow)
{
  m_pModel->SetAllowDragDrop(bAllow);
}

void ezQtDocumentTreeView::keyPressEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key::Key_Delete)
  {
    m_pDocument->DeleteSelectedObjects();
  }
  else
  {
    QTreeView::keyPressEvent(e);
  }
}

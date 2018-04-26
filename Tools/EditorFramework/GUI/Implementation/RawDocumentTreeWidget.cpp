#include <PCH.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <QKeyEvent>
#include <QSortFilterProxyModel>

ezQtDocumentTreeView::ezQtDocumentTreeView(QWidget* parent)
  : QTreeView(parent)
{

}

ezQtDocumentTreeView::ezQtDocumentTreeView(QWidget* pParent, ezDocument* pDocument, std::unique_ptr<ezQtDocumentTreeModel> pModel)
  : QTreeView(pParent)
{
  Initialize(pDocument, std::move(pModel));
}

void ezQtDocumentTreeView::Initialize(ezDocument* pDocument, std::unique_ptr<ezQtDocumentTreeModel> pModel)
{
  m_pDocument = pDocument;
  m_pModel = std::move(pModel);

  m_pFilterModel.reset(new ezQtTreeSearchFilterModel(this));
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
  setUniformRowHeights(true);

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

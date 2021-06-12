#include <EditorFrameworkPCH.h>

#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>

ezQtDocumentTreeView::ezQtDocumentTreeView(QWidget* parent)
  : QTreeView(parent)
{
}

ezQtDocumentTreeView::ezQtDocumentTreeView(QWidget* pParent, ezDocument* pDocument, std::unique_ptr<ezQtDocumentTreeModel> pModel, ezSelectionManager* pSelection)
  : QTreeView(pParent)
{
  Initialize(pDocument, std::move(pModel), pSelection);
}

void ezQtDocumentTreeView::Initialize(ezDocument* pDocument, std::unique_ptr<ezQtDocumentTreeModel> pModel, ezSelectionManager* pSelection)
{
  m_pDocument = pDocument;
  m_pModel = std::move(pModel);
  m_pSelectionManager = pSelection;
  if (m_pSelectionManager == nullptr)
  {
    // If no selection manager is provided, fall back to the default selection.
    m_pSelectionManager = m_pDocument->GetSelectionManager();
  }

  m_pFilterModel.reset(new ezQtTreeSearchFilterModel(this));
  m_pFilterModel->setSourceModel(m_pModel.get());

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

  EZ_VERIFY(connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
              SLOT(on_selectionChanged_triggered(const QItemSelection&, const QItemSelection&))) != nullptr,
    "signal/slot connection failed");
  m_pSelectionManager->m_Events.AddEventHandler(ezMakeDelegate(&ezQtDocumentTreeView::SelectionEventHandler, this));
}

ezQtDocumentTreeView::~ezQtDocumentTreeView()
{
  m_pSelectionManager->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtDocumentTreeView::SelectionEventHandler, this));
}

void ezQtDocumentTreeView::on_selectionChanged_triggered(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (m_bBlockSelectionSignal)
    return;

  QModelIndexList selection = selectionModel()->selectedIndexes();

  ezDeque<const ezDocumentObject*> sel;

  foreach (QModelIndex index, selection)
  {
    if (index.isValid())
    {
      index = m_pFilterModel->mapToSource(index);

      if (index.isValid())
        sel.PushBack((const ezDocumentObject*)index.internalPointer());
    }
  }

  // TODO const cast
  ((ezSelectionManager*)m_pSelectionManager)->SetSelection(sel);
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

      for (const ezDocumentObject* pObject : m_pSelectionManager->GetSelection())
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
  if (m_pSelectionManager->GetSelection().IsEmpty())
    return;

  const ezDocumentObject* pObject = m_pSelectionManager->GetSelection().PeekBack();

  auto index = m_pModel->ComputeModelIndex(pObject);
  index = m_pFilterModel->mapFromSource(index);

  if (index.isValid())
    scrollTo(index, QAbstractItemView::EnsureVisible);
}

void ezQtDocumentTreeView::SetAllowDragDrop(bool bAllow)
{
  m_pModel->SetAllowDragDrop(bAllow);
}

void ezQtDocumentTreeView::SetAllowDeleteObjects(bool bAllow)
{
  m_bAllowDeleteObjects = bAllow;
}

void ezQtDocumentTreeView::keyPressEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key::Key_Delete)
  {
    if (m_bAllowDeleteObjects)
    {
      m_pDocument->DeleteSelectedObjects();
    }
  }
  else
  {
    QTreeView::keyPressEvent(e);
  }
}

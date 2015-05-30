#include <PCH.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <QKeyEvent>

ezRawDocumentTreeWidget::ezRawDocumentTreeWidget(QWidget* pParent, const ezDocumentBase* pDocument) : 
  QTreeView(pParent),
  m_Model(pDocument->GetObjectTree()),
  m_pDocument(pDocument)
{
  m_bBlockSelectionSignal = false;
  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  setModel(&m_Model);
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);

  EZ_VERIFY(connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(on_selectionChanged_triggered(const QItemSelection&, const QItemSelection&))) != nullptr, "signal/slot connection failed");
  pDocument->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezRawDocumentTreeWidget::SelectionEventHandler, this));
  QModelIndexList selection = selectionModel()->selectedIndexes();
}

ezRawDocumentTreeWidget::~ezRawDocumentTreeWidget()
{
  m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezRawDocumentTreeWidget::SelectionEventHandler, this));
}

void ezRawDocumentTreeWidget::on_selectionChanged_triggered(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (m_bBlockSelectionSignal)
    return;

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

void ezRawDocumentTreeWidget::SelectionEventHandler(const ezSelectionManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezSelectionManager::Event::Type::SelectionCleared:
    {
      // Can't block signals on selection model or view won't update.
      m_bBlockSelectionSignal = true;
      selectionModel()->clear();
      m_bBlockSelectionSignal = false;
    }
    break;
  case ezSelectionManager::Event::Type::SelectionSet:
  case ezSelectionManager::Event::Type::ObjectAdded:
  case ezSelectionManager::Event::Type::ObjectRemoved:
    {
      // Can't block signals on selection model or view won't update.
      m_bBlockSelectionSignal = true;
      QItemSelection selection;
      
      for (const ezDocumentObjectBase* pObject : m_pDocument->GetSelectionManager()->GetSelection())
      {
        auto index = m_Model.ComputeModelIndex(pObject);
        selection.select(index, index);
      }
      selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows | QItemSelectionModel::NoUpdate);
      m_bBlockSelectionSignal = false;
    }
    break;
  }
}

void ezRawDocumentTreeWidget::keyPressEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key::Key_Delete)
  {
    auto objects = m_pDocument->GetSelectionManager()->GetSelection();

    auto history = m_pDocument->GetCommandHistory();
    history->StartTransaction();

    ezRemoveObjectCommand cmd;

    bool bCancel = false;
    for (const ezDocumentObjectBase* pObject : objects)
    {
      cmd.m_Object = pObject->GetGuid();

      if (history->AddCommand(cmd).m_Result.Failed())
      {
        bCancel = true;
        break;
      }
    }

    history->EndTransaction(bCancel);
  }
  else
  {
    QTreeView::keyPressEvent(e);
  }
}

#include <PCH.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <QKeyEvent>

ezQtDocumentTreeWidget::ezQtDocumentTreeWidget(QWidget* parent)
  : QTreeView(parent)
{

}

ezQtDocumentTreeWidget::ezQtDocumentTreeWidget(QWidget* pParent, ezDocument* pDocument, const ezRTTI* pBaseClass, const char* szChildProperty, std::unique_ptr<ezQtDocumentTreeModel> pCustomModel)
  : QTreeView(pParent)
{
  Initialize(pDocument, pBaseClass, szChildProperty, std::move(pCustomModel));
}

void ezQtDocumentTreeWidget::Initialize(ezDocument* pDocument, const ezRTTI* pBaseClass, const char* szChildProperty, std::unique_ptr<ezQtDocumentTreeModel> pCustomModel)
{
  m_pDocument = pDocument;

  if (pCustomModel)
    m_pModel = std::move(pCustomModel);
  else
    m_pModel.reset(new ezQtDocumentTreeModel(pDocument->GetObjectManager(), pBaseClass, szChildProperty));

  m_bBlockSelectionSignal = false;
  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  setModel(m_pModel.get());
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);
  setHeaderHidden(true);
  setExpandsOnDoubleClick(true);
  setEditTriggers(QAbstractItemView::EditTrigger::EditKeyPressed);

  EZ_VERIFY(connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(on_selectionChanged_triggered(const QItemSelection&, const QItemSelection&))) != nullptr, "signal/slot connection failed");
  pDocument->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtDocumentTreeWidget::SelectionEventHandler, this));
}

ezQtDocumentTreeWidget::~ezQtDocumentTreeWidget()
{
  m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtDocumentTreeWidget::SelectionEventHandler, this));
}

void ezQtDocumentTreeWidget::on_selectionChanged_triggered(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (m_bBlockSelectionSignal)
    return;

  QModelIndexList selection = selectionModel()->selectedIndexes();

  ezDeque<const ezDocumentObject*> sel;

  foreach(QModelIndex index, selection)
  {
    if (index.isValid())
      sel.PushBack((const ezDocumentObject*) index.internalPointer());
  }

  // TODO const cast
  ((ezSelectionManager*) m_pDocument->GetSelectionManager())->SetSelection(sel);
}

void ezQtDocumentTreeWidget::SelectionEventHandler(const ezSelectionManagerEvent& e)
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
        selection.select(index, index);
      }
      selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows | QItemSelectionModel::NoUpdate);
      m_bBlockSelectionSignal = false;
    }
    break;
  }
}

void ezQtDocumentTreeWidget::EnsureLastSelectedItemVisible()
{
  if (m_pDocument->GetSelectionManager()->GetSelection().IsEmpty())
    return;

  const ezDocumentObject* pObject = m_pDocument->GetSelectionManager()->GetSelection().PeekBack();

  auto index = m_pModel->ComputeModelIndex(pObject);
  scrollTo(index, QAbstractItemView::EnsureVisible);
}

void ezQtDocumentTreeWidget::keyPressEvent(QKeyEvent* e)
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

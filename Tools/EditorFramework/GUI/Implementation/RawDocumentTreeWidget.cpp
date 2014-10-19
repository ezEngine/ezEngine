#include <PCH.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>


ezRawDocumentTreeWidget::ezRawDocumentTreeWidget(QWidget* pParent, ezDocumentObjectTree* pTree) : 
  QTreeView(pParent),
  m_Model(pTree)
{
  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  setModel(&m_Model);
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);
}



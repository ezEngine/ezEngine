#include <PCH.h>
#include <GuiFoundation/Dialogs/PickDocumentObjectDlg.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Document/Document.h>

ezQtPickDocumentObjectDlg::ezQtPickDocumentObjectDlg(QWidget* parent, const ezArrayPtr<Element>& objects, const ezUuid& currentObject)
  : QDialog(parent)
  , m_Objects(objects)
  , m_CurrentObject(currentObject)
{
  setupUi(this);

  UpdateTable();
}


void ezQtPickDocumentObjectDlg::UpdateTable()
{
  QTreeWidget* pTree = ObjectTree;
  pTree->clear();
  pTree->setExpandsOnDoubleClick(false);

  ezMap<const ezDocumentObjectManager*, QTreeWidgetItem*> roots;

  ezStringBuilder name, temp;
  QTreeWidgetItem* pSelected = nullptr;

  for (auto& e : m_Objects)
  {
    const ezDocumentObjectManager* pManager = e.m_pObject->GetDocumentObjectManager();

    bool existed = false;
    auto itRootItem = roots.FindOrAdd(pManager, &existed);

    QTreeWidgetItem* pRoot = itRootItem.Value();

    if (!existed)
    {
      pRoot = new QTreeWidgetItem();
      itRootItem.Value() = pRoot;

      temp = pManager->GetDocument()->GetDocumentPath();
      name = temp.GetFileNameAndExtension();

      pTree->addTopLevelItem(pRoot);

      pRoot->setText(0, name.GetData());
      pRoot->setExpanded(true);
    }

    QTreeWidgetItem* pItem = new QTreeWidgetItem();
    pItem->setText(0, e.m_sDisplayName.GetData());
    pItem->setData(0, Qt::UserRole, qVariantFromValue<void*>((void*)e.m_pObject));

    pRoot->addChild(pItem);

    if (e.m_pObject->GetGuid() == m_CurrentObject)
    {
      pSelected = pItem;
    }
  }

  if (pSelected != nullptr)
  {
    pSelected->setSelected(true);
  }
}

void ezQtPickDocumentObjectDlg::on_ObjectTree_itemDoubleClicked(QTreeWidgetItem* pItem, int column)
{
  if (pItem->parent() == nullptr)
    return;

  QVariant var = pItem->data(0, Qt::UserRole);
  if (!var.isValid())
    return;

  m_pPickedObject = reinterpret_cast<const ezDocumentObject*>(var.value<void*>());

  if (m_pPickedObject != nullptr)
  {
    accept();
  }
}




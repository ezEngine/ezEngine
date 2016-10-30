#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>
#include <QTreeWidget>
#include <QBoxLayout>
#include <QStandardItemModel>


class QNullWidget : public QWidget
{
public:
  QNullWidget() : QWidget(nullptr) {}

protected:
  virtual void mousePressEvent(QMouseEvent* e) override
  {
    e->accept();
  }

  virtual void mouseReleaseEvent(QMouseEvent* e) override
  {
    e->accept();
  }

  virtual void mouseDoubleClickEvent(QMouseEvent* e) override
  {
    e->accept();
  }
};



ezQtSearchableMenu::ezQtSearchableMenu(QObject* parent)
  : QWidgetAction(parent)
{

  m_pGroup = new QNullWidget();
  m_pGroup->setLayout(new QVBoxLayout(m_pGroup));
  m_pGroup->setContentsMargins(0, 0, 0, 0);

  m_pSearch = new ezQtSearchWidget(m_pGroup);
  connect(m_pSearch, &ezQtSearchWidget::enterPressed, this, &ezQtSearchableMenu::OnEnterPressed);
  connect(m_pSearch, &ezQtSearchWidget::textChanged, this, &ezQtSearchableMenu::OnSearchChanged);

  m_pGroup->layout()->addWidget(m_pSearch);
  m_pGroup->layout()->setContentsMargins(1, 1, 1, 1);

  m_pItemModel = new QStandardItemModel(m_pGroup);

  m_pFilterModel = new ezQtTreeSearchFilterModel(m_pGroup);
  m_pFilterModel->setSourceModel(m_pItemModel);

  m_pTreeView = new QTreeView(m_pGroup);
  m_pTreeView->setModel(m_pFilterModel);
  m_pTreeView->setHeaderHidden(true);
  m_pTreeView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  m_pTreeView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  connect(m_pTreeView, &QTreeView::activated, this, &ezQtSearchableMenu::OnItemActivated);

  m_pGroup->layout()->addWidget(m_pTreeView);

  setDefaultWidget(m_pGroup);
}

QStandardItem* ezQtSearchableMenu::CreateCategoryMenu(const char* szCategory)
{
  if (ezStringUtils::IsNullOrEmpty(szCategory))
    return m_pItemModel->invisibleRootItem();

  auto it = m_Hierarchy.Find(szCategory);
  if (it.IsValid())
    return it.Value();

  ezStringBuilder sPath = szCategory;
  sPath.PathParentDirectory();
  sPath.Trim("/");

  QStandardItem* pParentMenu = m_pItemModel->invisibleRootItem();

  if (!sPath.IsEmpty())
  {
    pParentMenu = CreateCategoryMenu(sPath);
  }

  sPath = szCategory;
  sPath = sPath.GetFileName();

  QStandardItem* pThisItem = new QStandardItem(sPath.GetData());
  pThisItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);

  pParentMenu->appendRow(pThisItem);

  m_Hierarchy[szCategory] = pThisItem;

  return pThisItem;
}

void ezQtSearchableMenu::AddItem(const char* szName, const QVariant& variant)
{
  QStandardItem* pParent = m_pItemModel->invisibleRootItem();

  const char* szLastCat = ezStringUtils::FindLastSubString(szName, "/");
  if (szLastCat != nullptr)
  {
    ezStringBuilder sCategory;
    sCategory.SetSubString_FromTo(szName, szLastCat);

    pParent = CreateCategoryMenu(sCategory);

    szName = szLastCat + 1;
  }

  QStandardItem* pThisItem = new QStandardItem(szName);
  pThisItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
  pThisItem->setData(variant, Qt::UserRole + 1);

  pParent->appendRow(pThisItem);
}

QString ezQtSearchableMenu::GetSearchText() const
{
  return m_pSearch->text();
}

void ezQtSearchableMenu::Finalize(const QString& sSearchText)
{
  m_pItemModel->sort(0);

  m_pSearch->setText(sSearchText);
  m_pSearch->setFocus();
}

void ezQtSearchableMenu::OnItemActivated(const QModelIndex& index)
{
  if (!index.isValid())
    return;

  QModelIndex realIndex = m_pFilterModel->mapToSource(index);

  QString sName = m_pItemModel->data(realIndex, Qt::DisplayRole).toString();
  QVariant variant = m_pItemModel->data(realIndex, Qt::UserRole + 1);

  // potentially only a folder item
  if (!variant.isValid())
    return;

  emit MenuItemTriggered(sName, variant);
}

void ezQtSearchableMenu::OnEnterPressed()
{
  //auto selection = m_pTree->selectedItems();

  //if (selection.size() != 1)
  //  return;

  //OnItemActivated(selection[0], 0);
}

void ezQtSearchableMenu::OnSearchChanged(const QString& text)
{
  m_pFilterModel->SetFilterText(text);

  if (!text.isEmpty())
    m_pTreeView->expandAll();
}

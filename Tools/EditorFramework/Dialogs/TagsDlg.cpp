#include <PCH.h>
#include <EditorFramework/Dialogs/TagsDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <QInputDialog>
#include <QTreeWidget>

ezQtTagsDlg::ezQtTagsDlg(QWidget* parent) : QDialog(parent)
{
  setupUi(this);

  LoadTags();
  FillList();

  on_TreeTags_itemSelectionChanged();
}

void ezQtTagsDlg::on_ButtonNewCategory_clicked()
{
  QString sResult = QInputDialog::getText(this, "Category Name", "Name:");

  if (sResult.isEmpty())
    return;

  TreeTags->clearSelection();

  const ezString sName = sResult.toUtf8().data();

  if (m_CategoryToItem.Find(sName).IsValid())
  {
    ezQtUiServices::GetSingleton()->MessageBoxInformation("A Category with this name already exists.");
  }
  else
  {
    auto* pItem = new QTreeWidgetItem(TreeTags);
    pItem->setText(0, sResult);
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pItem->setIcon(0, ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Tag16.png"));

    m_CategoryToItem[sName] = pItem;
  }

  TreeTags->setItemSelected(m_CategoryToItem[sName], true);
}

void ezQtTagsDlg::on_ButtonNewTag_clicked()
{
  if (TreeTags->selectedItems().isEmpty())
    return;

  auto pItem = TreeTags->selectedItems()[0];

  if (!pItem)
    return;

  QString sResult = QInputDialog::getText(this, "Tag Name", "Name:");

  if (sResult.isEmpty())
    return;

  if (TreeTags->indexOfTopLevelItem(pItem) < 0)
    pItem = pItem->parent();

  auto pNewItem = CreateTagItem(pItem, sResult);
  pItem->setExpanded(true);

  TreeTags->clearSelection();
  TreeTags->setItemSelected(pNewItem, true);
  //TreeTags->editItem(pNewItem);
}

void ezQtTagsDlg::on_ButtonRemove_clicked()
{
  if (TreeTags->selectedItems().isEmpty())
    return;

  auto pItem = TreeTags->selectedItems()[0];

  if (!pItem)
    return;

  if (TreeTags->indexOfTopLevelItem(pItem) >= 0)
  {
    if (ezQtUiServices::GetSingleton()->MessageBoxQuestion("Do you really want to remove the entire Tag Category?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
      return;

    m_CategoryToItem.Remove(pItem->text(0).toUtf8().data());
  }
  else
  {
    if (ezQtUiServices::GetSingleton()->MessageBoxQuestion("Do you really want to remove this Tag?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
      return;
  }

  delete pItem;
}

void ezQtTagsDlg::on_ButtonOk_clicked()
{
  GetTagsFromList();
  SaveTags();
  accept();
}

void ezQtTagsDlg::on_ButtonCancel_clicked()
{
  reject();
}

void ezQtTagsDlg::on_ButtonReset_clicked()
{
  LoadTags();
  FillList();
  on_TreeTags_itemSelectionChanged();
}

void ezQtTagsDlg::on_TreeTags_itemSelectionChanged()
{
  const bool hasSelection = !TreeTags->selectedItems().isEmpty();

  ButtonRemove->setEnabled(hasSelection);
  ButtonNewTag->setEnabled(hasSelection);
}

void ezQtTagsDlg::LoadTags()
{
  m_Tags.Clear();

  ezHybridArray<const ezToolsTag*, 16> tags;
  ezToolsTagRegistry::GetAllTags(tags);

  for (const ezToolsTag* pTag : tags)
  {
    // hide the "Editor" tags from the user
    if (pTag->m_sCategory == "Editor")
      continue;

    auto& tag = m_Tags.ExpandAndGetRef();
    tag.m_sCategory = pTag->m_sCategory;
    tag.m_sName = pTag->m_sName;
  }
}


void ezQtTagsDlg::SaveTags()
{
  ezToolsTagRegistry::Clear();

  for (const auto& tag : m_Tags)
  {
    ezToolsTagRegistry::AddTag(tag);
  }

  ezQtEditorApp::GetSingleton()->SaveTagRegistry();
}

void ezQtTagsDlg::FillList()
{
  QtScopedBlockSignals bs(TreeTags);
  QtScopedUpdatesDisabled bu(TreeTags);

  m_CategoryToItem.Clear();
  TreeTags->clear();

  ezSet<ezString> TagCategories;

  for (const auto& tag : m_Tags)
  {
    TagCategories.Insert(tag.m_sCategory);
  }

  for (auto it = TagCategories.GetIterator(); it.IsValid(); ++it)
  {
    auto* pItem = new QTreeWidgetItem(TreeTags);
    pItem->setText(0, it.Key().GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pItem->setIcon(0, ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Tag16.png"));

    m_CategoryToItem[it.Key()] = pItem;
  }

  for (const auto& tag : m_Tags)
  {
    QTreeWidgetItem* pParentItem = m_CategoryToItem[tag.m_sCategory];

    CreateTagItem(pParentItem, QString::fromUtf8(tag.m_sName.GetData()));


    pParentItem->setExpanded(true);
  }

  TreeTags->resizeColumnToContents(0);
}

void ezQtTagsDlg::GetTagsFromList()
{
  m_Tags.Clear();

  for (int sets = 0; sets < TreeTags->topLevelItemCount(); ++sets)
  {
    const auto* pSetItem = TreeTags->topLevelItem(sets);
    const ezString sCategoryName = pSetItem->text(0).toUtf8().data();

    for (int children = 0; children < pSetItem->childCount(); ++children)
    {
      ezToolsTag& cfg = m_Tags.ExpandAndGetRef();
      cfg.m_sCategory = sCategoryName;

      auto* pTagItem = pSetItem->child(children);

      cfg.m_sName = pTagItem->text(0).toUtf8().data();
    }
  }
}

QTreeWidgetItem* ezQtTagsDlg::CreateTagItem(QTreeWidgetItem* pParentItem, const QString& tag)
{
  auto* pItem = new QTreeWidgetItem(pParentItem);
  pItem->setText(0, tag);
  pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsEditable);

  return pItem;
}
#include <PCH.h>
#include <EditorFramework/Dialogs/InputConfigDlg.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <QTreeWidget>
#include <QCheckBox>
#include <QMessageBox>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QSpinBox>
#include <QComboBox>
#include <QInputDialog>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <EditorApp/EditorApp.moc.h>
#include <qevent.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

void UpdateInputDynamicEnumValues()
{
  ezHybridArray<ezGameAppInputConfig, 32> Actions;

  ezStringBuilder sPath = ezToolsProject::GetInstance()->GetProjectPath();
  sPath.PathParentDirectory();
  sPath.AppendPath("InputConfig.json");

  ezFileReader file;
  if (file.Open(sPath).Failed())
    return;

  ezGameAppInputConfig::ReadFromJson(file, Actions);

  auto& dynEnum = ezDynamicStringEnum::GetDynamicEnum("InputSet");
  dynEnum.Clear();

  for (const auto& a : Actions)
  {
    dynEnum.AddValidValue(a.m_sInputSet, true);
  }
}

InputConfigDlg::InputConfigDlg(QWidget* parent) : QDialog(parent)
{
  setupUi(this);

  LoadActions();

  ezQtEditorApp::GetInstance()->GetKnownInputSlots(m_AllInputSlots);

  // make sure existing slots are always in the list
  // to prevent losing data when some plugin is not loaded
  {
    for (const auto& action : m_Actions)
    {
      for (int i = 0; i < 3; ++i)
      {
        if (m_AllInputSlots.IndexOf(action.m_sInputSlotTrigger[i]) == ezInvalidIndex)
          m_AllInputSlots.PushBack(action.m_sInputSlotTrigger[i]);
      }
    }
  }

  FillList();

  on_TreeActions_itemSelectionChanged();
}

void InputConfigDlg::on_ButtonNewInputSet_clicked()
{
  QString sResult = QInputDialog::getText(this, "Input Set Name", "Name:");

  if (sResult.isEmpty())
    return;

  TreeActions->clearSelection();

  const ezString sName = sResult.toUtf8().data();

  if (m_InputSetToItem.Find(sName).IsValid())
  {
    ezUIServices::GetInstance()->MessageBoxInformation("An Input Set with this name already exists.");
  }
  else
  {
    auto* pItem = new QTreeWidgetItem(TreeActions);
    pItem->setText(0, sResult);
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pItem->setIcon(0, ezUIServices::GetInstance()->GetCachedIconResource(":/EditorFramework/Icons/Input16.png"));

    m_InputSetToItem[sName] = pItem;
  }

  TreeActions->setItemSelected(m_InputSetToItem[sName], true);
}

void InputConfigDlg::on_ButtonNewAction_clicked()
{
  if (TreeActions->selectedItems().isEmpty())
    return;

  auto pItem = TreeActions->selectedItems()[0];

  if (!pItem)
    return;

  if (TreeActions->indexOfTopLevelItem(pItem) < 0)
    pItem = pItem->parent();

  ezGameAppInputConfig action;
  auto pNewItem = CreateActionItem(pItem, action);
  pItem->setExpanded(true);

  TreeActions->clearSelection();
  TreeActions->setItemSelected(pNewItem, true);
  TreeActions->editItem(pNewItem);
}

void InputConfigDlg::on_ButtonRemove_clicked()
{
  if (TreeActions->selectedItems().isEmpty())
    return;

  auto pItem = TreeActions->selectedItems()[0];

  if (!pItem)
    return;

  if (TreeActions->indexOfTopLevelItem(pItem) >= 0)
  {
    if (ezUIServices::GetInstance()->MessageBoxQuestion("Do you really want to remove the entire Input Set?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
      return;

    m_InputSetToItem.Remove(pItem->text(0).toUtf8().data());
  }
  else
  {
    if (ezUIServices::GetInstance()->MessageBoxQuestion("Do you really want to remove this action?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
      return;
  }

  delete pItem;
}

void InputConfigDlg::on_ButtonOk_clicked()
{
  GetActionsFromList();
  SaveActions();
  UpdateInputDynamicEnumValues();
  accept();
}

void InputConfigDlg::on_ButtonCancel_clicked()
{
  reject();
}

void InputConfigDlg::on_ButtonReset_clicked()
{
  LoadActions();
  FillList();
  on_TreeActions_itemSelectionChanged();
}

void InputConfigDlg::on_TreeActions_itemSelectionChanged()
{
  const bool hasSelection = !TreeActions->selectedItems().isEmpty();

  ButtonRemove->setEnabled(hasSelection);
  ButtonNewAction->setEnabled(hasSelection);
}

void InputConfigDlg::LoadActions()
{
  m_Actions.Clear();

  ezStringBuilder sPath = ezToolsProject::GetInstance()->GetProjectPath();
  sPath.PathParentDirectory();
  sPath.AppendPath("InputConfig.json");

  ezFileReader file;
  if (file.Open(sPath).Failed())
    return;

  ezGameAppInputConfig::ReadFromJson(file, m_Actions);
}

void InputConfigDlg::SaveActions()
{
  ezStringBuilder sPath = ezToolsProject::GetInstance()->GetProjectPath();
  sPath.PathParentDirectory();
  sPath.AppendPath("InputConfig.json");

  ezFileWriter file;
  if (file.Open(sPath).Failed())
    return;

  ezGameAppInputConfig::WriteToJson(file, m_Actions);
}

void InputConfigDlg::FillList()
{
  QtScopedBlockSignals bs(TreeActions);
  QtScopedUpdatesDisabled bu(TreeActions);

  m_InputSetToItem.Clear();
  TreeActions->clear();

  ezSet<ezString> InputSets;

  for (const auto& action : m_Actions)
  {
    InputSets.Insert(action.m_sInputSet);
  }

  for (auto it = InputSets.GetIterator(); it.IsValid(); ++it)
  {
    auto* pItem = new QTreeWidgetItem(TreeActions);
    pItem->setText(0, it.Key().GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pItem->setIcon(0, ezUIServices::GetInstance()->GetCachedIconResource(":/EditorFramework/Icons/Input16.png"));

    m_InputSetToItem[it.Key()] = pItem;
  }

  for (const auto& action : m_Actions)
  {
    QTreeWidgetItem* pParentItem = m_InputSetToItem[action.m_sInputSet];

    CreateActionItem(pParentItem, action);


    pParentItem->setExpanded(true);
  }

  TreeActions->resizeColumnToContents(0);
  TreeActions->resizeColumnToContents(1);
  TreeActions->resizeColumnToContents(2);
  TreeActions->resizeColumnToContents(3);
  TreeActions->resizeColumnToContents(4);
  TreeActions->resizeColumnToContents(5);
  TreeActions->resizeColumnToContents(6);
  TreeActions->resizeColumnToContents(7);
}

void InputConfigDlg::GetActionsFromList()
{
  m_Actions.Clear();

  for (int sets = 0; sets < TreeActions->topLevelItemCount(); ++sets)
  {
    const auto* pSetItem = TreeActions->topLevelItem(sets);
    const ezString sSetName = pSetItem->text(0).toUtf8().data();

    for (int children = 0; children < pSetItem->childCount(); ++children)
    {
      ezGameAppInputConfig& cfg = m_Actions.ExpandAndGetRef();
      cfg.m_sInputSet = sSetName;

      auto* pActionItem = pSetItem->child(children);

      cfg.m_sInputAction = pActionItem->text(0).toUtf8().data();
      cfg.m_bApplyTimeScaling = qobject_cast<QCheckBox*>(TreeActions->itemWidget(pActionItem, 1))->isChecked();
      
      for (int i = 0; i < 3; ++i)
      {
        cfg.m_sInputSlotTrigger[i] = qobject_cast<QComboBox*>(TreeActions->itemWidget(pActionItem, 2 + i * 2))->currentText().toUtf8().data();
        cfg.m_fInputSlotScale[i] = qobject_cast<QDoubleSpinBox*>(TreeActions->itemWidget(pActionItem, 3 + i * 2))->value();
      }
    }
  }

}

QTreeWidgetItem* InputConfigDlg::CreateActionItem(QTreeWidgetItem* pParentItem, const ezGameAppInputConfig& action)
{
  auto* pItem = new QTreeWidgetItem(pParentItem);
  pItem->setText(0, action.m_sInputAction.GetData());
  pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsEditable);

  QCheckBox* pTimeScale = new QCheckBox(TreeActions);
  pTimeScale->setChecked(action.m_bApplyTimeScaling);
  TreeActions->setItemWidget(pItem, 1, pTimeScale);

  for (int i = 0; i < 3; ++i)
  {
    QDoubleSpinBox* spin = new QDoubleSpinBox(TreeActions);
    spin->setDecimals(3);
    spin->setMinimum(0.0);
    spin->setMaximum(100.0);
    spin->setSingleStep(0.01);
    spin->setValue(action.m_fInputSlotScale[i]);

    TreeActions->setItemWidget(pItem, 3 + 2 * i, spin);

    QComboBox* combo = new QComboBox(TreeActions);
    combo->setAutoCompletion(true);
    combo->setAutoCompletionCaseSensitivity(Qt::CaseInsensitive);
    combo->setEditable(true);
    combo->setInsertPolicy(QComboBox::InsertAtBottom);
    combo->setMaxVisibleItems(15);

    for (ezUInt32 it = 0; it < m_AllInputSlots.GetCount(); ++it)
    {
      combo->addItem(m_AllInputSlots[it].GetData());
    }

    int index = combo->findText(action.m_sInputSlotTrigger[i].GetData());

    if (index > 0)
      combo->setCurrentIndex(index);
    else
      combo->setCurrentIndex(0);

    TreeActions->setItemWidget(pItem, 2 + 2 * i, combo);
  }

  return pItem;
}





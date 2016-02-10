#include <PCH.h>
#include <EditorFramework/Dialogs/InputConfigDlg.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <QTreeWidget>
#include <QCheckBox>

InputConfigDlg::InputConfigDlg(QWidget* parent) : QDialog(parent)
{
  setupUi(this);

  LoadActions();
  FillList();
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

void InputConfigDlg::FillList()
{
  QtScopedBlockSignals bs(TreeActions);
  QtScopedUpdatesDisabled bu(TreeActions);

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

    m_InputSetToItem[it.Key()] = pItem;
  }

  for (const auto& action : m_Actions)
  {
    QTreeWidgetItem* pParentItem = m_InputSetToItem[action.m_sInputSet];

    auto* pItem = new QTreeWidgetItem(pParentItem);
    pItem->setText(0, action.m_sInputAction.GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsEditable);
    
    QCheckBox* pTimeScale = new QCheckBox(TreeActions);
    pTimeScale->setChecked(action.m_bApplyTimeScaling);
    TreeActions->setItemWidget(pItem, 1, pTimeScale);

  }
}






#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/PluginSelectionWidget.moc.h>
#include <EditorFramework/EditorApp/Configuration/Plugins.h>

ezQtPluginSelectionWidget::ezQtPluginSelectionWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setupUi(this);
}

ezQtPluginSelectionWidget::~ezQtPluginSelectionWidget() = default;

void ezQtPluginSelectionWidget::SetPluginSet(ezPluginBundleSet* pPluginSet)
{
  m_pPluginSet = pPluginSet;
  m_States.Clear();

  ezSet<ezString> templates;
  ezStringBuilder tmp;

  for (auto& plugin : m_pPluginSet->m_Plugins)
  {
    auto& pi = plugin.Value();

    for (const auto& tmp : pi.m_EnabledInTemplates)
    {
      templates.Insert(tmp);
    }

    if (pi.m_bMandatory)
      continue;

    auto& state = m_States.ExpandAndGetRef();
    state.m_sID = plugin.Key();
    state.m_pInfo = &pi;
    state.m_bSelected = pi.m_bSelected;
    state.m_bLoadCopy = pi.m_bLoadCopy;

    tmp = pi.m_sDisplayName;

    QListWidgetItem* pItem = new QListWidgetItem();

    if (pi.m_bMissing)
    {
      tmp.Append(" (missing)");
      pItem->setBackground(QColor::fromRgb(200, 0, 0));
    }

    pItem->setText(tmp.GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);
    pItem->setCheckState(pi.m_bSelected ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    pItem->setData(Qt::UserRole, plugin.Key().GetData());

    PluginsList->addItem(pItem);
  }

  on_PluginsList_currentItemChanged(nullptr, nullptr);

  Template->clear();
  Template->addItem("<custom>");

  for (const ezString tmp : templates)
  {
    Template->addItem(tmp.GetData());
  }

  UpdateInternalState();
}

void ezQtPluginSelectionWidget::SyncStateToSet()
{
  // make sure to pull the latest state from the UI
  on_PluginsList_currentItemChanged(nullptr, PluginsList->currentItem());

  for (auto& state : m_States)
  {
    state.m_pInfo->m_bSelected = state.m_bSelected;
    state.m_pInfo->m_bLoadCopy = state.m_bLoadCopy;
  }
}

void ezQtPluginSelectionWidget::on_PluginsList_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
  if (previous)
  {
    auto& state = m_States[PluginsList->row(previous)];
    state.m_bLoadCopy = LoadCopy->isChecked();
  }

  if (current)
  {
    auto& state = m_States[PluginsList->row(current)];
    DescriptionText->setPlainText(state.m_pInfo->m_sDescription.GetData());
    LoadCopy->setChecked(state.m_bLoadCopy);
    LoadCopy->setEnabled(true);
  }
  else
  {
    LoadCopy->setEnabled(false);
  }
}

void ezQtPluginSelectionWidget::on_PluginsList_itemChanged(QListWidgetItem* item)
{
  auto& state = m_States[PluginsList->row(item)];
  state.m_bSelected = item->checkState() == Qt::Checked;

  Template->setCurrentIndex(0); // <custom>

  UpdateInternalState();
}

void ezQtPluginSelectionWidget::on_Template_currentIndexChanged(int index)
{
  if (index <= 0)
    return;

  const ezString sTemplate = Template->currentText().toUtf8().data();

  for (auto& s : m_States)
  {
    s.m_bSelected = s.m_pInfo->m_EnabledInTemplates.Contains(sTemplate);
  }

  UpdateInternalState();
}

void ezQtPluginSelectionWidget::UpdateInternalState()
{
  for (auto& s : m_States)
  {
    s.m_bIsDependency = false;
  }

  for (auto& s : m_States)
  {
    if (s.m_bSelected)
    {
      ApplyRequired(s.m_pInfo->m_RequiredBundles);
    }
  }

  PluginsList->blockSignals(true);

  ezSet<ezString, ezCompareString_NoCase> exclusiveFeatures;

  for (ezUInt32 row = 0; row < m_States.GetCount(); ++row)
  {
    const auto& pi = m_States[row];
    auto pItem = PluginsList->item(row);

    if (pi.m_bSelected || pi.m_bIsDependency)
    {
      for (const auto& feat : pi.m_pInfo->m_ExclusiveFeatures)
      {
        exclusiveFeatures.Insert(feat);
      }
    }

    if (pi.m_bIsDependency)
      pItem->setFlags(Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);
    else
      pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);

    pItem->setCheckState(pi.m_bSelected || pi.m_bIsDependency ? Qt::Checked : Qt::Unchecked);
  }

  for (ezUInt32 row = 0; row < m_States.GetCount(); ++row)
  {
    const auto& pi = m_States[row];
    auto pItem = PluginsList->item(row);

    if (pi.m_bSelected || pi.m_bIsDependency)
      continue;

    for (const auto& feat : pi.m_pInfo->m_ExclusiveFeatures)
    {
      if (exclusiveFeatures.Contains(feat))
      {
        goto disable;
      }
    }

    continue;

  disable:
    pItem->setFlags(Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);
  }

  PluginsList->blockSignals(false);
}

void ezQtPluginSelectionWidget::ApplyRequired(ezArrayPtr<ezString> required)
{
  for (const auto& reqName : required)
  {
    for (auto& s : m_States)
    {
      if (s.m_sID == reqName)
      {
        if (!s.m_bIsDependency)
        {
          ApplyRequired(s.m_pInfo->m_RequiredBundles);
        }

        s.m_bIsDependency = true;
        break;
      }
    }
  }
}

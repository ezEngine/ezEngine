#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/Dialogs/JoltProjectSettingsDlg.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <QCheckBox>
#include <QInputDialog>

void UpdateCollisionLayerDynamicEnumValues();
void UpdateWeightCategoryDynamicEnumValues();

ezQtJoltProjectSettingsDlg::ezQtJoltProjectSettingsDlg(const ezVariant& startup, QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  ButtonRemoveLayer->setEnabled(false);
  ButtonRenameLayer->setEnabled(false);

  EnsureConfigFileExists();

  Load().IgnoreResult();
  SetupFilterTable();
  SetupWeightTable();

  Tabs->setCurrentIndex(0);

  if (startup.IsValid())
  {
    const ezString sStartup = startup.ConvertTo<ezString>();
    if (sStartup == "WeightCategories")
    {
      Tabs->setCurrentIndex(1);
    }
  }
}

void ezQtJoltProjectSettingsDlg::EnsureConfigFileExists()
{
  EnsureFilterConfigFileExists();
  EnsureWeightsConfigFileExists();
}

void ezQtJoltProjectSettingsDlg::EnsureFilterConfigFileExists()
{
  if (ezFileSystem::ExistsFile(ezCollisionFilterConfig::s_sConfigFile))
    return;

  ezCollisionFilterConfig cfg;

  cfg.SetGroupName(0, "Default");
  cfg.SetGroupName(1, "Transparent");
  cfg.SetGroupName(2, "Debris");
  cfg.SetGroupName(3, "Water");
  cfg.SetGroupName(4, "Foliage");
  cfg.SetGroupName(5, "AI");
  cfg.SetGroupName(6, "Player");
  cfg.SetGroupName(7, "Visibility Raycast");
  cfg.SetGroupName(8, "Interaction Raycast");

  cfg.EnableCollision(0, 0);
  cfg.EnableCollision(0, 1);
  cfg.EnableCollision(0, 2);
  cfg.EnableCollision(0, 3, false);
  cfg.EnableCollision(0, 4, false);
  cfg.EnableCollision(0, 5);
  cfg.EnableCollision(0, 6);
  cfg.EnableCollision(0, 7);
  cfg.EnableCollision(0, 8);

  cfg.EnableCollision(1, 1);
  cfg.EnableCollision(1, 2);
  cfg.EnableCollision(1, 3, false);
  cfg.EnableCollision(1, 4, false);
  cfg.EnableCollision(1, 5);
  cfg.EnableCollision(1, 6);
  cfg.EnableCollision(1, 7, false);
  cfg.EnableCollision(1, 8);

  cfg.EnableCollision(2, 2, false);
  cfg.EnableCollision(2, 3, false);
  cfg.EnableCollision(2, 4, false);
  cfg.EnableCollision(2, 5, false);
  cfg.EnableCollision(2, 6, false);
  cfg.EnableCollision(2, 7, false);
  cfg.EnableCollision(2, 8, false);

  cfg.EnableCollision(3, 3, false);
  cfg.EnableCollision(3, 4, false);
  cfg.EnableCollision(3, 5, false);
  cfg.EnableCollision(3, 6, false);
  cfg.EnableCollision(3, 7, false);
  cfg.EnableCollision(3, 8);

  cfg.EnableCollision(4, 4, false);
  cfg.EnableCollision(4, 5, false);
  cfg.EnableCollision(4, 6, false);
  cfg.EnableCollision(4, 7);
  cfg.EnableCollision(4, 8, false);

  cfg.EnableCollision(5, 5);
  cfg.EnableCollision(5, 6);
  cfg.EnableCollision(5, 7);
  cfg.EnableCollision(5, 8);

  cfg.EnableCollision(6, 6);
  cfg.EnableCollision(6, 7);
  cfg.EnableCollision(6, 8);

  cfg.EnableCollision(7, 7, false);
  cfg.EnableCollision(7, 8, false);

  cfg.EnableCollision(8, 8, false);

  cfg.Save().IgnoreResult();

  UpdateCollisionLayerDynamicEnumValues();
}

void ezQtJoltProjectSettingsDlg::SetupFilterTable()
{
  ezQtScopedBlockSignals s1(FilterTable);
  ezQtScopedUpdatesDisabled s2(FilterTable);

  const ezUInt32 uiLayers = m_Config.GetNumNamedGroups();

  FilterTable->setRowCount(uiLayers);
  FilterTable->setColumnCount(uiLayers);
  FilterTable->horizontalHeader()->setHighlightSections(false);

  QStringList headers;
  ezStringBuilder tmp;

  for (ezUInt32 r = 0; r < uiLayers; ++r)
  {
    m_IndexRemap[r] = m_Config.GetNamedGroupIndex(r);

    headers.push_back(QString::fromUtf8(m_Config.GetGroupName(m_IndexRemap[r]).GetData(tmp)));
  }

  FilterTable->setVerticalHeaderLabels(headers);
  FilterTable->setHorizontalHeaderLabels(headers);

  for (ezUInt32 r = 0; r < uiLayers; ++r)
  {
    for (ezUInt32 c = 0; c < uiLayers; ++c)
    {
      QCheckBox* pCheck = new QCheckBox();
      pCheck->setText(QString());
      pCheck->setChecked(m_Config.IsCollisionEnabled(m_IndexRemap[r], m_IndexRemap[c]));
      pCheck->setProperty("column", c);
      pCheck->setProperty("row", r);
      connect(pCheck, &QCheckBox::clicked, this, &ezQtJoltProjectSettingsDlg::onCheckBoxClicked);

      QWidget* pWidget = new QWidget();
      QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
      pLayout->addWidget(pCheck);
      pLayout->setAlignment(Qt::AlignCenter);
      pLayout->setContentsMargins(0, 0, 0, 0);
      pWidget->setLayout(pLayout);

      FilterTable->setCellWidget(r, c, pWidget);
    }
  }
}

ezResult ezQtJoltProjectSettingsDlg::Save()
{
  if (m_Config.Save().Failed())
  {
    ezStringBuilder sError;
    sError.SetFormat("Failed to save the Collision Layer file\n'{0}'", ezCollisionFilterConfig::s_sConfigFile);

    ezQtUiServices::GetSingleton()->MessageBoxWarning(sError);

    return EZ_FAILURE;
  }

  UpdateCollisionLayerDynamicEnumValues();

  if (m_WeightConfig.Save().Failed())
  {
    ezStringBuilder sError;
    sError.SetFormat("Failed to save the Weight Categories file\n'{0}'", ezWeightCategoryConfig::s_sConfigFile);

    ezQtUiServices::GetSingleton()->MessageBoxWarning(sError);

    return EZ_FAILURE;
  }

  UpdateWeightCategoryDynamicEnumValues();

  return EZ_SUCCESS;
}

ezResult ezQtJoltProjectSettingsDlg::Load()
{
  EZ_SUCCEED_OR_RETURN(m_Config.Load());
  EZ_SUCCEED_OR_RETURN(m_WeightConfig.Load());

  m_ConfigReset = m_Config;
  m_WeightConfigReset = m_WeightConfig;
  return EZ_SUCCESS;
}

static void AddWeightCfg(ezWeightCategoryConfig& ref_cfg, ezStringView sName, float fMass, ezStringView sDesc)
{
  ezUInt8 idx = ref_cfg.GetFreeKey();
  auto& e = ref_cfg.m_Categories[idx];
  e.m_sName.Assign(sName);
  e.m_fMass = fMass;
  e.m_sDescription = sDesc;
}

void ezQtJoltProjectSettingsDlg::EnsureWeightsConfigFileExists()
{
  if (ezFileSystem::ExistsFile(ezWeightCategoryConfig::s_sConfigFile))
    return;

  ezWeightCategoryConfig cfg;
  AddWeightCfg(cfg, "Barrel", 35.0f, "");
  AddWeightCfg(cfg, "Car", 500.0f, "");
  AddWeightCfg(cfg, "Chair", 10.0f, "");
  AddWeightCfg(cfg, "Crate - Large", 100.0f, "larger than 1 meter");
  AddWeightCfg(cfg, "Crate - Medium", 40.0f, "up to 1 meter");
  AddWeightCfg(cfg, "Crate - Small", 10.0f, "smaller than 0.5 meters");
  AddWeightCfg(cfg, "Creature - Small", 10.0f, "small animals, rats, birds");
  AddWeightCfg(cfg, "Creature - Medium", 70.0f, "Humanoids, regular monsters");
  AddWeightCfg(cfg, "Creature - Large", 150.0f, "Large monsters");
  AddWeightCfg(cfg, "Debris", 1.0f, "tiny objects, cans, garbage");
  AddWeightCfg(cfg, "Decoration - Small", 2.0f, "picture frames, plates, mugs");
  AddWeightCfg(cfg, "Decoration - Medium", 5.0f, "paintings, desk lamps, regular vases");
  AddWeightCfg(cfg, "Decoration - Large", 8.0f, "ceiling lamps, large vases");
  AddWeightCfg(cfg, "Furniture - Large", 80.0f, "shelves, tables, large cupboards");
  AddWeightCfg(cfg, "Furniture - Medium", 25.0f, "sideboards, cupboards, small tables");
  AddWeightCfg(cfg, "Furniture - Small", 10.0f, "stools, bedside tables");
  AddWeightCfg(cfg, "Truck", 1000.0f, "trucks, trains, containers, large machinery");

  cfg.Save().IgnoreResult();

  UpdateWeightCategoryDynamicEnumValues();
}

void ezQtJoltProjectSettingsDlg::onCheckBoxClicked(bool checked)
{
  QCheckBox* pCheck = qobject_cast<QCheckBox*>(sender());

  const ezInt32 c = pCheck->property("column").toInt();
  const ezInt32 r = pCheck->property("row").toInt();

  m_Config.EnableCollision(m_IndexRemap[c], m_IndexRemap[r], pCheck->isChecked());

  if (r != c)
  {
    QCheckBox* pCheck2 = qobject_cast<QCheckBox*>(FilterTable->cellWidget(c, r)->layout()->itemAt(0)->widget());
    pCheck2->setChecked(pCheck->isChecked());
  }
}

void ezQtJoltProjectSettingsDlg::on_DefaultButtons_clicked(QAbstractButton* pButton)
{
  if (pButton == DefaultButtons->button(QDialogButtonBox::Ok))
  {
    if (Save().Failed())
      return;

    accept();
    return;
  }

  if (pButton == DefaultButtons->button(QDialogButtonBox::Cancel))
  {
    reject();
    return;
  }

  if (pButton == DefaultButtons->button(QDialogButtonBox::Reset))
  {
    m_Config = m_ConfigReset;
    m_WeightConfig = m_WeightConfigReset;
    SetupFilterTable();
    SetupWeightTable();
    return;
  }
}

void ezQtJoltProjectSettingsDlg::on_ButtonAddLayer_clicked()
{
  const ezUInt32 uiNewIdx = m_Config.FindUnnamedGroup();

  if (uiNewIdx == ezInvalidIndex)
  {
    ezQtUiServices::GetSingleton()->MessageBoxInformation("The maximum number of collision layers has been reached.");
    return;
  }

  while (true)
  {
    bool ok;
    QString result = QInputDialog::getText(this, QStringLiteral("Add Layer"), QStringLiteral("Name:"), QLineEdit::Normal, QString(), &ok);

    if (!ok)
      return;

    if (m_Config.GetFilterGroupByName(result.toUtf8().data()) != ezInvalidIndex)
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning("A Collision Layer with the given name already exists.");
      continue;
    }

    m_Config.SetGroupName(uiNewIdx, result.toUtf8().data());
    break;
  }

  SetupFilterTable();
}

void ezQtJoltProjectSettingsDlg::on_ButtonRemoveLayer_clicked()
{
  if (ezQtUiServices::GetSingleton()->MessageBoxQuestion("Remove selected Collision Layer?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::No)
    return;

  const auto sel = FilterTable->selectionModel()->selectedRows();

  if (sel.isEmpty())
    return;

  const int iRow = sel[0].row();

  m_Config.SetGroupName(m_IndexRemap[iRow], "");

  SetupFilterTable();

  FilterTable->clearSelection();
}

void ezQtJoltProjectSettingsDlg::on_ButtonRenameLayer_clicked()
{
  const auto sel = FilterTable->selectionModel()->selectedRows();

  if (sel.isEmpty())
    return;

  const int iGroupIdx = m_IndexRemap[sel[0].row()];
  const ezString sOldName = m_Config.GetGroupName(iGroupIdx);

  m_Config.SetGroupName(iGroupIdx, "");

  while (true)
  {
    bool ok;
    QString result = QInputDialog::getText(this, QStringLiteral("Rename Layer"), QStringLiteral("Name:"), QLineEdit::Normal, QString::fromUtf8(sOldName.GetData()), &ok);

    if (!ok)
    {
      m_Config.SetGroupName(iGroupIdx, sOldName);
      return;
    }

    if (m_Config.GetFilterGroupByName(result.toUtf8().data()) != ezInvalidIndex)
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning("A Collision Layer with the given name already exists.");
      continue;
    }

    m_Config.SetGroupName(iGroupIdx, result.toUtf8().data());
    SetupFilterTable();

    return;
  }
}

void ezQtJoltProjectSettingsDlg::on_FilterTable_itemSelectionChanged()
{
  const auto sel = FilterTable->selectionModel()->selectedRows();
  ButtonRemoveLayer->setEnabled(!sel.isEmpty());
  ButtonRenameLayer->setEnabled(!sel.isEmpty());
}

void ezQtJoltProjectSettingsDlg::on_ButtonAddCategory_clicked()
{
  ezHashedString name;

  const ezUInt8 uiFreeKey = m_WeightConfig.GetFreeKey();
  if (uiFreeKey == 255)
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("You managed to create too many categories.");
    return;
  }

  while (true)
  {
    bool ok;
    QString result = QInputDialog::getText(this, QStringLiteral("Add Weight Category"), QStringLiteral("Name:"), QLineEdit::Normal, QString(), &ok);

    if (!ok)
      return;

    if (m_WeightConfig.FindByName(result.toUtf8().data()) != 255)
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning("A weight category with that name already exists.");
      continue;
    }

    m_WeightConfig.m_Categories[uiFreeKey].m_sName.Assign(result.toUtf8().data());
    break;
  }

  SetupWeightTable();
}

void ezQtJoltProjectSettingsDlg::on_ButtonRemoveCategory_clicked()
{
  if (ezQtUiServices::GetSingleton()->MessageBoxQuestion("Remove selected category?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::No)
    return;

  const auto sel = WeightsTable->selectionModel()->selectedIndexes();

  if (sel.isEmpty())
    return;

  const ezUInt32 uiRow = sel[0].row();

  const ezUInt8 idx = m_WeightConfig.FindByName(m_RowToWeight[uiRow]);
  m_WeightConfig.m_Categories.RemoveAndCopy(idx);

  SetupWeightTable();
}

void ezQtJoltProjectSettingsDlg::on_ButtonRenameCategory_clicked()
{
  const auto sel = WeightsTable->selectionModel()->selectedRows();

  if (sel.isEmpty())
    return;

  const ezHashedString sOldCatName = m_RowToWeight[sel[0].row()];
  const ezUInt8 uiCatIdx = m_WeightConfig.FindByName(sOldCatName);

  if (uiCatIdx == 255)
    return;

  m_WeightConfig.m_Categories[uiCatIdx].m_sName.Assign("-tmp-");

  while (true)
  {
    bool ok;
    QString result = QInputDialog::getText(this, QStringLiteral("Rename Category"), QStringLiteral("Name:"), QLineEdit::Normal, QString::fromUtf8(sOldCatName.GetString().GetData()), &ok);

    if (!ok)
    {
      m_WeightConfig.m_Categories[uiCatIdx].m_sName = sOldCatName;
      return;
    }

    if (m_WeightConfig.FindByName(result.toUtf8().data()) != 255)
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning("A weight category with the given name already exists.");
      continue;
    }

    m_WeightConfig.m_Categories[uiCatIdx].m_sName.Assign(result.toUtf8().data());
    SetupWeightTable();
    return;
  }
}

void ezQtJoltProjectSettingsDlg::SetupWeightTable()
{
  ezQtScopedBlockSignals s1(WeightsTable);
  ezQtScopedUpdatesDisabled s2(WeightsTable);

  ezMap<ezString, ezWeightCategory, ezCompareString_NoCase> sorted;

  m_WeightConfig.m_Categories.Sort();
  const ezUInt32 uiRows = m_WeightConfig.m_Categories.GetCount();

  for (ezUInt32 r = 0; r < uiRows; ++r)
  {
    const auto& cat = m_WeightConfig.m_Categories.GetPair(r);
    sorted[cat.value.m_sName.GetString()] = cat.value;
  }

  WeightsTable->clear();
  WeightsTable->setColumnCount(3);
  WeightsTable->setHorizontalHeaderLabels({"Name", "Mass", "Description"});
  WeightsTable->setRowCount(uiRows);

  m_RowToWeight.SetCount(uiRows);

  ezUInt32 uiRow = 0;
  for (const auto cat : sorted)
  {
    m_RowToWeight[uiRow].Assign(cat.Key());

    WeightsTable->setItem(uiRow, 0, new QTableWidgetItem(ezMakeQString(cat.Key())));

    ezQtDoubleSpinBox* pNumber = new ezQtDoubleSpinBox(nullptr);
    pNumber->setMinimum(1.0);
    pNumber->setMaximum(1000);
    pNumber->setDecimals(1);
    pNumber->setValue(cat.Value().m_fMass);

    pNumber->setProperty("category", ezMakeQString(cat.Key()));
    connect(pNumber, &ezQtDoubleSpinBox::valueChanged, this, &ezQtJoltProjectSettingsDlg::onWeightChanged);

    QWidget* pWidget = new QWidget();
    QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
    pLayout->addWidget(pNumber);
    pLayout->setAlignment(Qt::AlignCenter);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pWidget->setLayout(pLayout);

    WeightsTable->setCellWidget(uiRow, 1, pWidget);

    QLineEdit* pDesc = new QLineEdit();
    pDesc->setText(ezMakeQString(cat.Value().m_sDescription));
    pDesc->setProperty("category", ezMakeQString(cat.Key()));
    connect(pDesc, &QLineEdit::textChanged, this, &ezQtJoltProjectSettingsDlg::onWeightDescChanged);

    WeightsTable->setCellWidget(uiRow, 2, pDesc);

    ++uiRow;
  }
}

void ezQtJoltProjectSettingsDlg::onWeightChanged(double value)
{
  ezQtDoubleSpinBox* pNumber = qobject_cast<ezQtDoubleSpinBox*>(sender());

  const ezString sCat = pNumber->property("category").toString().toUtf8().data();
  const ezUInt8 idx = m_WeightConfig.FindByName(sCat);

  m_WeightConfig.m_Categories[idx].m_fMass = static_cast<float>(value);
}

void ezQtJoltProjectSettingsDlg::onWeightDescChanged(const QString& txt)
{
  QLineEdit* pEdit = qobject_cast<QLineEdit*>(sender());

  const ezString sCat = pEdit->property("category").toString().toUtf8().data();
  const ezUInt8 idx = m_WeightConfig.FindByName(sCat);

  m_WeightConfig.m_Categories[idx].m_sDescription = txt.toUtf8().data();
}

#include <PCH.h>
#include <EditorPluginPhysX/Dialogs/PhysXProjectSettingsDlg.moc.h>
#include <GameUtils/CollisionFilter/CollisionFilter.h>
#include <GuiFoundation/Basics.h>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Core/Application/Config/ApplicationConfig.h>

void UpdateCollisionLayerDynamicEnumValues();

ezQtPhysxProjectSettingsDlg::ezQtPhysxProjectSettingsDlg(QWidget* parent) : QDialog(parent)
{
  setupUi(this);

  ButtonRemoveLayer->setEnabled(false);
  ButtonRenameLayer->setEnabled(false);

  Load();
  SetupTable();
}

void ezQtPhysxProjectSettingsDlg::SetupTable()
{
  QtScopedBlockSignals s1(FilterTable);
  QtScopedUpdatesDisabled s2(FilterTable);

  const ezUInt32 uiLayers = m_Config.GetNumNamedGroups();

  FilterTable->setRowCount(uiLayers);
  FilterTable->setColumnCount(uiLayers);
  FilterTable->horizontalHeader()->setHighlightSections(false);

  QStringList headers;

  for (ezUInt32 r = 0; r < uiLayers; ++r)
  {
    m_IndexRemap[r] = m_Config.GetNamedGroupIndex(r);

    headers.push_back(QString::fromUtf8(m_Config.GetGroupName(m_IndexRemap[r])));
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
      connect(pCheck, &QCheckBox::clicked, this, &ezQtPhysxProjectSettingsDlg::onCheckBoxClicked);

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

ezResult ezQtPhysxProjectSettingsDlg::Save()
{
  ezStringBuilder sPath = ezApplicationConfig::GetProjectDirectory();
  sPath.AppendPath("Physics/CollisionLayers.cfg");

  if (m_Config.Save(sPath).Failed())
  {
    ezStringBuilder sError;
    sError.Format("Failed to save the Collision Layer file\n'%s'", sPath.GetData());

    ezQtUiServices::GetSingleton()->MessageBoxWarning(sError);

    return EZ_FAILURE;
  }

  UpdateCollisionLayerDynamicEnumValues();

  return EZ_SUCCESS;
}

ezResult ezQtPhysxProjectSettingsDlg::Load()
{
  ezStringBuilder sPath = ezApplicationConfig::GetProjectDirectory();
  sPath.AppendPath("Physics/CollisionLayers.cfg");

  auto res = m_Config.Load(sPath);

  if (res.Failed())
  {
    m_Config.SetGroupName(0, "Default");
    m_Config.SetGroupName(1, "Transparent");
    m_Config.SetGroupName(2, "Debris");
    m_Config.SetGroupName(3, "Water");
    m_Config.SetGroupName(4, "Foliage");
    m_Config.SetGroupName(5, "AI");
    m_Config.SetGroupName(6, "Player");
    m_Config.SetGroupName(7, "Visibility Raycast");
    m_Config.SetGroupName(8, "Interaction Raycast");

    m_Config.EnableCollision(0, 0);
    m_Config.EnableCollision(0, 1);
    m_Config.EnableCollision(0, 2);
    m_Config.EnableCollision(0, 3, false);
    m_Config.EnableCollision(0, 4, false);
    m_Config.EnableCollision(0, 5);
    m_Config.EnableCollision(0, 6);
    m_Config.EnableCollision(0, 7);
    m_Config.EnableCollision(0, 8);

    m_Config.EnableCollision(1, 1);
    m_Config.EnableCollision(1, 2);
    m_Config.EnableCollision(1, 3, false);
    m_Config.EnableCollision(1, 4, false);
    m_Config.EnableCollision(1, 5);
    m_Config.EnableCollision(1, 6);
    m_Config.EnableCollision(1, 7, false);
    m_Config.EnableCollision(1, 8);

    m_Config.EnableCollision(2, 2, false);
    m_Config.EnableCollision(2, 3, false);
    m_Config.EnableCollision(2, 4, false);
    m_Config.EnableCollision(2, 5, false);
    m_Config.EnableCollision(2, 6, false);
    m_Config.EnableCollision(2, 7, false);
    m_Config.EnableCollision(2, 8, false);

    m_Config.EnableCollision(3, 3, false);
    m_Config.EnableCollision(3, 4, false);
    m_Config.EnableCollision(3, 5, false);
    m_Config.EnableCollision(3, 6, false);
    m_Config.EnableCollision(3, 7, false);
    m_Config.EnableCollision(3, 8);

    m_Config.EnableCollision(4, 4, false);
    m_Config.EnableCollision(4, 5, false);
    m_Config.EnableCollision(4, 6, false);
    m_Config.EnableCollision(4, 7);
    m_Config.EnableCollision(4, 8, false);

    m_Config.EnableCollision(5, 5);
    m_Config.EnableCollision(5, 6);
    m_Config.EnableCollision(5, 7);
    m_Config.EnableCollision(5, 8);

    m_Config.EnableCollision(6, 6);
    m_Config.EnableCollision(6, 7);
    m_Config.EnableCollision(6, 8);

    m_Config.EnableCollision(7, 7, false);
    m_Config.EnableCollision(7, 8, false);

    m_Config.EnableCollision(8, 8, false);
  }

  m_ConfigReset = m_Config;
  return res;
}

void ezQtPhysxProjectSettingsDlg::onCheckBoxClicked(bool checked)
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

void ezQtPhysxProjectSettingsDlg::on_DefaultButtons_clicked(QAbstractButton* pButton)
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
    SetupTable();
    return;
  }
}

void ezQtPhysxProjectSettingsDlg::on_ButtonAddLayer_clicked()
{
  const ezInt32 iNewIdx = m_Config.FindUnnamedGroup();

  if (iNewIdx < 0)
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

    if (m_Config.GetFilterGroupByName(result.toUtf8().data()) >= 0)
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning("A Collision Layer with the given name already exists.");
      continue;
    }

    m_Config.SetGroupName(iNewIdx, result.toUtf8().data());
    break;
  }

  SetupTable();
}

void ezQtPhysxProjectSettingsDlg::on_ButtonRemoveLayer_clicked()
{
  if (ezQtUiServices::GetSingleton()->MessageBoxQuestion("Remove selected Collision Layer?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::No)
    return;

  const auto sel = FilterTable->selectionModel()->selectedRows();

  if (sel.isEmpty())
    return;

  const int iRow = sel[0].row();

  m_Config.SetGroupName(m_IndexRemap[iRow], "");

  SetupTable();

  FilterTable->clearSelection();
}

void ezQtPhysxProjectSettingsDlg::on_ButtonRenameLayer_clicked()
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

    if (m_Config.GetFilterGroupByName(result.toUtf8().data()) >= 0)
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning("A Collision Layer with the given name already exists.");
      continue;
    }

    m_Config.SetGroupName(iGroupIdx, result.toUtf8().data());
    SetupTable();

    return;
  }
}

void ezQtPhysxProjectSettingsDlg::on_FilterTable_itemSelectionChanged()
{
  const auto sel = FilterTable->selectionModel()->selectedRows();
  ButtonRemoveLayer->setEnabled(!sel.isEmpty());
  ButtonRenameLayer->setEnabled(!sel.isEmpty());
}


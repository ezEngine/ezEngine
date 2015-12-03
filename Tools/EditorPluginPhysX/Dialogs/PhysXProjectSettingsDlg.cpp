#include <PCH.h>
#include <EditorPluginPhysX/Dialogs/PhysXProjectSettingsDlg.moc.h>
#include <GameUtils/CollisionFilter/CollisionFilter.h>
#include <GuiFoundation/Basics.h>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QInputDialog>

ezPhysxProjectSettingsDlg::ezPhysxProjectSettingsDlg(ezCollisionFilterConfig* pFilterCfg, QWidget* parent) : QDialog(parent)
{
  m_pFilterCfg = pFilterCfg;
  m_Config = *pFilterCfg;

  setupUi(this);

  SetupTable();
}

void ezPhysxProjectSettingsDlg::SetupTable()
{
  QtScopedBlockSignals s1(FilterTable);
  QtScopedUpdatesDisabled s2(FilterTable);

  const ezUInt32 uiLayers = m_Config.GetFilterGroupCount();

  FilterTable->setRowCount(uiLayers);
  FilterTable->setColumnCount(uiLayers);
  FilterTable->horizontalHeader()->setHighlightSections(false);

  QStringList headers;

  for (ezUInt32 r = 0; r < uiLayers; ++r)
  {
    headers.push_back(QString::fromUtf8(m_Config.GetGroupName(r)));
  }

  FilterTable->setVerticalHeaderLabels(headers);
  FilterTable->setHorizontalHeaderLabels(headers);

  for (ezUInt32 r = 0; r < uiLayers; ++r)
  {
    for (ezUInt32 c = 0; c < uiLayers; ++c)
    {
      QCheckBox* pCheck = new QCheckBox();
      pCheck->setText(QString());
      pCheck->setChecked(m_Config.IsCollisionEnabled(r, c));
      pCheck->setProperty("column", c);
      pCheck->setProperty("row", r);
      connect(pCheck, &QCheckBox::clicked, this, &ezPhysxProjectSettingsDlg::onCheckBoxClicked);

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

void ezPhysxProjectSettingsDlg::onCheckBoxClicked(bool checked)
{
  QCheckBox* pCheck = qobject_cast<QCheckBox*>(sender());

  const ezInt32 c = pCheck->property("column").toInt();
  const ezInt32 r = pCheck->property("row").toInt();

  m_Config.EnableCollision(c, r, pCheck->isChecked());

  if (r != c)
  {
    QCheckBox* pCheck2 = qobject_cast<QCheckBox*>(FilterTable->cellWidget(c, r)->layout()->itemAt(0)->widget());
    pCheck2->setChecked(pCheck->isChecked());
  }
}

void ezPhysxProjectSettingsDlg::on_DefaultButtons_clicked(QAbstractButton* pButton)
{
  if (pButton == DefaultButtons->button(QDialogButtonBox::Ok))
  {
    /// \todo Sync with engine process

    *m_pFilterCfg = m_Config;
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
    m_Config = *m_pFilterCfg;
    SetupTable();
    return;
  }
}

void ezPhysxProjectSettingsDlg::on_ButtonAddLayer_clicked()
{
  bool ok;
  QString result = QInputDialog::getText(this, QStringLiteral("Add Layer"), QStringLiteral("Name:"), QLineEdit::Normal, QString(), &ok);

  if (!ok)
    return;

  m_Config.SetFilterGroupCount(m_Config.GetFilterGroupCount() + 1);
  m_Config.SetGroupName(m_Config.GetFilterGroupCount() - 1, result.toUtf8().data());

  SetupTable();
}

void ezPhysxProjectSettingsDlg::on_ButtonRemoveLayer_clicked()
{
  /// \todo Have 'unused' groups
}


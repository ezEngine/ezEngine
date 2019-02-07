#include <PCH.h>

#include <Foundation/Configuration/CVar.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/CVarWidget.moc.h>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>

ezQtCVarWidget::ezQtCVarWidget(QWidget* parent)
    : QWidget(parent)
{
  setupUi(this);
}

ezQtCVarWidget::~ezQtCVarWidget() {}

void ezQtCVarWidget::Clear()
{
  clearFocus();
  TableCVars->clear();

  {
    QStringList Headers;
    Headers.append("");
    Headers.append(" Plugin ");
    Headers.append(" CVar ");
    Headers.append(" Value ");
    Headers.append(" Description ");

    TableCVars->setColumnCount(Headers.size());

    TableCVars->setHorizontalHeaderLabels(Headers);
    TableCVars->horizontalHeader()->show();
  }
}

void ezQtCVarWidget::RebuildCVarUI(const ezMap<ezString, ezCVarWidgetData>& cvars)
{
  {
    ezQtScopedBlockSignals bs(TableCVars);

    TableCVars->clear();
    TableCVars->setRowCount(cvars.GetCount());

    QStringList Headers;
    Headers.append("");
    Headers.append(" Plugin ");
    Headers.append(" CVar ");
    Headers.append(" Value ");
    Headers.append(" Description ");

    TableCVars->setColumnCount(Headers.size());

    TableCVars->setHorizontalHeaderLabels(Headers);
    TableCVars->horizontalHeader()->show();
    TableCVars->setSortingEnabled(false); // this does not work with updating the data later, would need to use a QTableView instead

    ezStringBuilder sTemp;

    QPixmap icon = ezQtUiServices::GetCachedPixmapResource(":/GuiFoundation/Icons/CVar.png");

    ezInt32 iRow = 0;
    for (auto it = cvars.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_iTableRow = iRow;

      QLabel* pIcon = new QLabel();
      pIcon->setPixmap(icon);
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TableCVars->setCellWidget(iRow, 0, pIcon);

      // Plugin
      sTemp.Format("  {0}  ", it.Value().m_sPlugin);
      TableCVars->setItem(iRow, 1, new QTableWidgetItem(sTemp.GetData()));

      // Name
      sTemp.Format("  {0}  ", it.Key());
      TableCVars->setItem(iRow, 2, new QTableWidgetItem(sTemp.GetData()));

      // // Description
      QTableWidgetItem* pDesc = new QTableWidgetItem(it.Value().m_sDescription.GetData());
      pDesc->setToolTip(it.Value().m_sDescription.GetData());
      TableCVars->setItem(iRow, 4, pDesc);

      switch (it.Value().m_uiType)
      {
        case ezCVarType::Bool:
        {
          QComboBox* pValue = new QComboBox;
          pValue->setProperty("cvar", it.Key().GetData());
          pValue->addItem("true");
          pValue->addItem("false");
          TableCVars->setCellWidget(iRow, 3, pValue); // Value

          QWidget::connect(pValue, SIGNAL(currentIndexChanged(int)), this, SLOT(BoolChanged(int)));
        }
        break;
        case ezCVarType::Float:
        {
          QDoubleSpinBox* pValue = new QDoubleSpinBox;
          pValue->setProperty("cvar", it.Key().GetData());
          pValue->setMinimum(-(1 << 30));
          pValue->setMaximum((1 << 30));
          pValue->setDecimals(4);
          pValue->setSingleStep(1.0);
          TableCVars->setCellWidget(iRow, 3, pValue); // Value

          QWidget::connect(pValue, SIGNAL(editingFinished()), this, SLOT(FloatChanged()));
        }
        break;
        case ezCVarType::Int:
        {
          QSpinBox* pValue = new QSpinBox;
          pValue->setProperty("cvar", it.Key().GetData());
          pValue->setMinimum(-(1 << 30));
          pValue->setMaximum((1 << 30));
          TableCVars->setCellWidget(iRow, 3, pValue); // Value

          QWidget::connect(pValue, SIGNAL(editingFinished()), this, SLOT(IntChanged()));
        }
        break;
        case ezCVarType::String:
        {
          QLineEdit* pValue = new QLineEdit;
          pValue->setProperty("cvar", it.Key().GetData());
          TableCVars->setCellWidget(iRow, 3, pValue); // Value

          QWidget::connect(pValue, SIGNAL(editingFinished()), this, SLOT(StringChanged()));
        }
        break;
      }

      ++iRow;
    }

    TableCVars->resizeColumnsToContents();
  }

  UpdateCVarUI(cvars);
}

void ezQtCVarWidget::UpdateCVarUI(const ezMap<ezString, ezCVarWidgetData>& cvars)
{

  for (auto it = cvars.GetIterator(); it.IsValid(); ++it)
  {
    const ezInt32 iRow = it.Value().m_iTableRow;

    // Value
    {
      switch (it.Value().m_uiType)
      {
        case ezCVarType::Bool:
        {
          QComboBox* pValue = (QComboBox*)TableCVars->cellWidget(iRow, 3);
          ezQtScopedBlockSignals bs(pValue);
          pValue->setCurrentIndex(it.Value().m_bValue ? 0 : 1);
        }
        break;
        case ezCVarType::Float:
        {
          QDoubleSpinBox* pValue = (QDoubleSpinBox*)TableCVars->cellWidget(iRow, 3);
          ezQtScopedBlockSignals bs(pValue);
          pValue->setValue(it.Value().m_fValue);
        }
        break;
        case ezCVarType::Int:
        {
          QSpinBox* pValue = (QSpinBox*)TableCVars->cellWidget(iRow, 3);
          ezQtScopedBlockSignals bs(pValue);
          pValue->setValue(it.Value().m_iValue);
        }
        break;
        case ezCVarType::String:
        {
          QLineEdit* pValue = (QLineEdit*)TableCVars->cellWidget(iRow, 3);
          ezQtScopedBlockSignals bs(pValue);
          pValue->setText(it.Value().m_sValue.GetData());
        }
        break;
      }
    }
  }
}

void ezQtCVarWidget::BoolChanged(int index)
{
  const QComboBox* pValue = qobject_cast<QComboBox*>(sender());
  const QString cvar = pValue->property("cvar").toString();

  const bool newValue = (pValue->currentIndex() == 0);

  Q_EMIT onBoolChanged(cvar.toUtf8().data(), newValue);
}

void ezQtCVarWidget::FloatChanged()
{
  const QDoubleSpinBox* pValue = qobject_cast<QDoubleSpinBox*>(sender());
  const QString cvar = pValue->property("cvar").toString();

  const float newValue = (float)pValue->value();

  Q_EMIT onFloatChanged(cvar.toUtf8().data(), newValue);
}

void ezQtCVarWidget::IntChanged()
{
  const QSpinBox* pValue = qobject_cast<QSpinBox*>(sender());
  const QString cvar = pValue->property("cvar").toString();

  const int newValue = (int)pValue->value();

  Q_EMIT onIntChanged(cvar.toUtf8().data(), newValue);
}

void ezQtCVarWidget::StringChanged()
{
  const QLineEdit* pValue = qobject_cast<QLineEdit*>(sender());
  const QString cvar = pValue->property("cvar").toString();

  const QString newValue = pValue->text();

  Q_EMIT onStringChanged(cvar.toUtf8().data(), newValue.toUtf8().data());
}

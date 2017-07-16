#include <PCH.h>
#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Widgets/CVarWidget.moc.h>
#include <Foundation/Configuration/CVar.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>

ezQtCVarWidget::ezQtCVarWidget(QWidget* parent)
  : QWidget(parent)
{
  setupUi(this);

}

ezQtCVarWidget::~ezQtCVarWidget()
{
}

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

    ezStringBuilder sTemp;

    ezInt32 iRow = 0;
    for (auto it = cvars.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_iTableRow = iRow;

      QLabel* pIcon = new QLabel();
      pIcon->setPixmap(ezQtUiServices::GetCachedPixmapResource(":/GuiFoundation/Icons/CVar.png"));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TableCVars->setCellWidget(iRow, 0, pIcon);

      sTemp.Format("  {0}  ", it.Value().m_sPlugin);
      TableCVars->setCellWidget(iRow, 1, new QLabel(sTemp.GetData())); // Plugin

      sTemp.Format("  {0}  ", it.Key());
      TableCVars->setCellWidget(iRow, 2, new QLabel(sTemp.GetData())); // Name

      TableCVars->setCellWidget(iRow, 4, new QLabel(it.Value().m_sDescription.GetData())); // Description

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

          QWidget::connect(pValue, SIGNAL(valueChanged(double)), this, SLOT(FloatChanged(double)));
        }
        break;
      case ezCVarType::Int:
        {
          QSpinBox* pValue = new QSpinBox;
          pValue->setProperty("cvar", it.Key().GetData());
          pValue->setMinimum(-(1 << 30));
          pValue->setMaximum((1 << 30));
          TableCVars->setCellWidget(iRow, 3, pValue); // Value

          QWidget::connect(pValue, SIGNAL(valueChanged(int)), this, SLOT(IntChanged(int)));
        }
        break;
      case ezCVarType::String:
        {
          QLineEdit* pValue = new QLineEdit;
          pValue->setProperty("cvar", it.Key().GetData());
          TableCVars->setCellWidget(iRow, 3, pValue); // Value

          QWidget::connect(pValue, SIGNAL(textChanged(const QString&)), this, SLOT(StringChanged(const QString&)));
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
  ezInt32 iRow = 0;
  for (auto it = cvars.GetIterator(); it.IsValid(); ++it)
  {
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

    ++iRow;
  }
}

void ezQtCVarWidget::BoolChanged(int index)
{
  const QComboBox* pValue = qobject_cast<QComboBox*>(sender());
  const QString cvar = pValue->property("cvar").toString();

  const bool newValue = (pValue->currentIndex() == 0);

  emit onBoolChanged(cvar.toUtf8().data(), newValue);
}

void ezQtCVarWidget::FloatChanged(double val)
{
  const QDoubleSpinBox* pValue = qobject_cast<QDoubleSpinBox*>(sender());
  const QString cvar = pValue->property("cvar").toString();

  const float newValue = (float)pValue->value();

  emit onFloatChanged(cvar.toUtf8().data(), newValue);
}

void ezQtCVarWidget::IntChanged(int val)
{
  const QSpinBox* pValue = qobject_cast<QSpinBox*>(sender());
  const QString cvar = pValue->property("cvar").toString();

  const int newValue = (int)pValue->value();

  emit onIntChanged(cvar.toUtf8().data(), newValue);
}

void ezQtCVarWidget::StringChanged(const QString& val)
{
  const QLineEdit* pValue = qobject_cast<QLineEdit*>(sender());
  const QString cvar = pValue->property("cvar").toString();

  const QString newValue = pValue->text();

  emit onStringChanged(cvar.toUtf8().data(), newValue.toUtf8().data());
}

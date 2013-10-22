#include <Inspector/CVarsWidget.moc.h>
#include <qlistwidget.h>
#include <Foundation/Communication/Telemetry.h>
#include <MainWindow.moc.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qspinbox.h>

ezCVarsWidget* ezCVarsWidget::s_pWidget = NULL;

ezCVarsWidget::ezCVarsWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

  ResetStats();
}

void ezCVarsWidget::ResetStats()
{
  m_CVars.Clear();
  TableCVars->clear();

  {
    TableCVars->setColumnCount(4);

    QStringList Headers;
    Headers.append(" Plugin ");
    Headers.append(" CVar ");
    Headers.append(" Value ");
    Headers.append(" Description ");

    TableCVars->setHorizontalHeaderLabels(Headers);
    TableCVars->horizontalHeader()->show();
  }
}

void ezCVarsWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage msg;

  bool bUpdateCVarsTable = false;
  bool bFillCVarsTable = false;

  while (ezTelemetry::RetrieveMessage('CVAR', msg) == EZ_SUCCESS)
  {
    if (msg.GetMessageID() == 'CLR')
    {
      s_pWidget->m_CVars.Clear();
    }

    if (msg.GetMessageID() == 'DATA')
    {
      ezString sName;
      msg.GetReader() >> sName;

      CVarData& sd = s_pWidget->m_CVars[sName];

      msg.GetReader() >> sd.m_sPlugin;
      msg.GetReader() >> sd.m_uiFlags;
      msg.GetReader() >> sd.m_uiType;
      msg.GetReader() >> sd.m_sDescription;

      switch (sd.m_uiType)
      {
      case ezCVarType::Bool:
        msg.GetReader() >> sd.m_bValue;
        break;
      case ezCVarType::Float:
        msg.GetReader() >> sd.m_fValue;
        break;
      case ezCVarType::Int:
        msg.GetReader() >> sd.m_iValue;
        break;
      case ezCVarType::String:
        msg.GetReader() >> sd.m_sValue;
        break;
      }

      if (sd.m_iTableRow == -1)
        bUpdateCVarsTable = true;

      bFillCVarsTable = true;
    }
  }

  if (bUpdateCVarsTable)
    s_pWidget->UpdateCVarsTable(true);
  else
  if (bFillCVarsTable)
    s_pWidget->UpdateCVarsTable(false);

}

void ezCVarsWidget::UpdateCVarsTable(bool bRecreate)
{
  TableCVars->blockSignals(true);

  if (bRecreate)
  {
    TableCVars->clear();
    TableCVars->setColumnCount(4);
    TableCVars->setRowCount(m_CVars.GetCount());

    QStringList Headers;
    Headers.append(" Plugin ");
    Headers.append(" CVar ");
    Headers.append(" Value ");
    Headers.append(" Description ");

    TableCVars->setHorizontalHeaderLabels(Headers);
    TableCVars->horizontalHeader()->show();

    ezStringBuilder sTemp;

    ezInt32 iRow = 0;
    for (ezMap<ezString, CVarData>::Iterator it = m_CVars.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_iTableRow = iRow;

      sTemp.Format("  %s  ", it.Value().m_sPlugin.GetData());
      TableCVars->setCellWidget(iRow, 0, new QLabel(sTemp.GetData())); // Plugin

      sTemp.Format("  %s  ", it.Key().GetData());
      TableCVars->setCellWidget(iRow, 1, new QLabel(sTemp.GetData())); // Name

      TableCVars->setCellWidget(iRow, 3, new QLabel(it.Value().m_sDescription.GetData())); // Description

      switch (it.Value().m_uiType)
      {
      case ezCVarType::Bool:
        {
          QComboBox* pValue = new QComboBox;
          pValue->addItem("true");
          pValue->addItem("false");
          TableCVars->setCellWidget(iRow, 2, pValue); // Value

          QWidget::connect(pValue, SIGNAL(currentIndexChanged(int)), this, SLOT(BoolChanged(int)));
        }
        break;
      case ezCVarType::Float:
        {
          QDoubleSpinBox* pValue = new QDoubleSpinBox;
          pValue->setMinimum(-(1 << 30));
          pValue->setMaximum( (1 << 30));
          pValue->setDecimals(4);
          pValue->setSingleStep(1.0);
          TableCVars->setCellWidget(iRow, 2, pValue); // Value        

          QWidget::connect(pValue, SIGNAL(valueChanged(double)), this, SLOT(FloatChanged(double)));
        }
        break;
      case ezCVarType::Int:
        {
          QSpinBox* pValue = new QSpinBox;
          pValue->setMinimum(-(1 << 30));
          pValue->setMaximum( (1 << 30));
          TableCVars->setCellWidget(iRow, 2, pValue); // Value

          QWidget::connect(pValue, SIGNAL(valueChanged(int)), this, SLOT(IntChanged(int)));
        }
        break;
      case ezCVarType::String:
        {
          QLineEdit* pValue = new QLineEdit;
          TableCVars->setCellWidget(iRow, 2, pValue); // Value

          QWidget::connect(pValue, SIGNAL(textChanged (const QString&)), this, SLOT(StringChanged(const QString&)));
        }
        break;
      }

      ++iRow;
    }

    TableCVars->resizeColumnsToContents();
  }

  TableCVars->blockSignals(false);

  {
    ezInt32 iRow = 0;
    for (ezMap<ezString, CVarData>::Iterator it = m_CVars.GetIterator(); it.IsValid(); ++it)
    {
      // Value
      {
        switch (it.Value().m_uiType)
        {
        case ezCVarType::Bool:
          {
            QComboBox* pValue = (QComboBox*) TableCVars->cellWidget(iRow, 2);
            pValue->blockSignals(true);
            pValue->setCurrentIndex(it.Value().m_bValue ? 0 : 1);
            pValue->blockSignals(false);
          }
          break;
        case ezCVarType::Float:
          {
            QDoubleSpinBox* pValue = (QDoubleSpinBox*) TableCVars->cellWidget(iRow, 2);
            pValue->blockSignals(true);
            pValue->setValue(it.Value().m_fValue);
            pValue->blockSignals(false);
          }
          break;
        case ezCVarType::Int:
          {
            QSpinBox* pValue = (QSpinBox*) TableCVars->cellWidget(iRow, 2);
            pValue->blockSignals(true);
            pValue->setValue(it.Value().m_iValue);
            pValue->blockSignals(false);
          }
          break;
        case ezCVarType::String:
          {
            QLineEdit* pValue = (QLineEdit*) TableCVars->cellWidget(iRow, 2);
            pValue->blockSignals(true);
            pValue->setText(it.Value().m_sValue.GetData());
            pValue->blockSignals(false);
          }
          break;
        }
      }

      ++iRow;
    }
  }

  
}


void ezCVarsWidget::BoolChanged(int index)
{
  for (ezMap<ezString, CVarData>::Iterator it = m_CVars.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_uiType != ezCVarType::Bool)
      continue;

    QComboBox* pValue = (QComboBox*) TableCVars->cellWidget(it.Value().m_iTableRow, 2);

    const ezInt32 iValue = it.Value().m_bValue ? 0 : 1;

    if (pValue->currentIndex() == iValue) // index 0 is 'true', index 1 is 'false'
      continue;

    it.Value().m_bValue = (pValue->currentIndex() == 0);

    ezTelemetryMessage Msg;
    Msg.SetMessageID('SVAR', 'SET');
    Msg.GetWriter() << it.Key().GetData();
    Msg.GetWriter() << it.Value().m_uiType;
    Msg.GetWriter() << it.Value().m_bValue;

    ezTelemetry::SendToServer(Msg);
  }
}

void ezCVarsWidget::FloatChanged(double val)
{
  for (ezMap<ezString, CVarData>::Iterator it = m_CVars.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_uiType != ezCVarType::Float)
      continue;

    QDoubleSpinBox* pValue = (QDoubleSpinBox*) TableCVars->cellWidget(it.Value().m_iTableRow, 2);

    if (pValue->value() == it.Value().m_fValue)
      continue;

    it.Value().m_fValue = (float) pValue->value();

    ezTelemetryMessage Msg;
    Msg.SetMessageID('SVAR', 'SET');
    Msg.GetWriter() << it.Key().GetData();
    Msg.GetWriter() << it.Value().m_uiType;
    Msg.GetWriter() << it.Value().m_fValue;

    ezTelemetry::SendToServer(Msg);
  }
}

void ezCVarsWidget::IntChanged(int val)
{
  for (ezMap<ezString, CVarData>::Iterator it = m_CVars.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_uiType != ezCVarType::Int)
      continue;

    QSpinBox* pValue = (QSpinBox*) TableCVars->cellWidget(it.Value().m_iTableRow, 2);

    if (pValue->value() == it.Value().m_iValue)
      continue;

    it.Value().m_iValue = pValue->value();

    ezTelemetryMessage Msg;
    Msg.SetMessageID('SVAR', 'SET');
    Msg.GetWriter() << it.Key().GetData();
    Msg.GetWriter() << it.Value().m_uiType;
    Msg.GetWriter() << it.Value().m_iValue;

    ezTelemetry::SendToServer(Msg);
  }
}

void ezCVarsWidget::StringChanged(const QString& val)
{
  for (ezMap<ezString, CVarData>::Iterator it = m_CVars.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_uiType != ezCVarType::String)
      continue;

    QLineEdit* pValue = (QLineEdit*) TableCVars->cellWidget(it.Value().m_iTableRow, 2);

    if (pValue->text().toUtf8().data() == it.Value().m_sValue)
      continue;

    it.Value().m_sValue = pValue->text().toUtf8().data();

    ezTelemetryMessage Msg;
    Msg.SetMessageID('SVAR', 'SET');
    Msg.GetWriter() << it.Key().GetData();
    Msg.GetWriter() << it.Value().m_uiType;
    Msg.GetWriter() << it.Value().m_sValue;

    ezTelemetry::SendToServer(Msg);
  }

}

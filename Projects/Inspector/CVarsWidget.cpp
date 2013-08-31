#include <Inspector/CVarsWidget.moc.h>
#include <qlistwidget.h>
#include <Foundation/Communication/Telemetry.h>
#include <MainWindow.moc.h>

ezCVarsWidget* ezCVarsWidget::s_pWidget = NULL;

ezCVarsWidget::ezCVarsWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

  ResetStats();
}

void ezCVarsWidget::ResetStats()
{
}

void ezCVarsWidget::UpdateStats()
{
}

void ezCVarsWidget::ProcessTelemetry_CVars(void* pPassThrough)
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
    TableCVars->setColumnCount(3);
    TableCVars->setRowCount(m_CVars.GetCount());

    QStringList Headers;
    Headers.append(" Plugin ");
    Headers.append(" CVar ");
    Headers.append(" Value ");

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

      TableCVars->setCellWidget(iRow, 2, new QLabel("  ??????  ")); // Value

      ++iRow;
    }

    TableCVars->resizeColumnsToContents();
  }

  {
    ezStringBuilder sTemp;

    ezInt32 iRow = 0;
    for (ezMap<ezString, CVarData>::Iterator it = m_CVars.GetIterator(); it.IsValid(); ++it)
    {
      // Value
      {
        QLabel* pValue = (QLabel*) TableCVars->cellWidget(iRow, 2);

        switch (it.Value().m_uiType)
        {
        case ezCVarType::Bool:
          pValue->setText(it.Value().m_bValue ? "  true  " : "  false  ");
          break;
        case ezCVarType::Float:
          sTemp.Format("  %.3f  ", it.Value().m_fValue);
          pValue->setText(sTemp.GetData());
          break;
        case ezCVarType::Int:
          sTemp.Format("  %i  ", it.Value().m_iValue);
          pValue->setText(sTemp.GetData());
          break;
        case ezCVarType::String:
          sTemp.Format("  %s  ", it.Value().m_sValue.GetData());
          pValue->setText(sTemp.GetData());
          break;
        }
      }

      ++iRow;
    }
  }

  
  TableCVars->blockSignals(false);
}


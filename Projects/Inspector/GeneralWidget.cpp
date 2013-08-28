#include <Inspector/GeneralWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/GeneralWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <qinputdialog.h>

ezGeneralWidget* ezGeneralWidget::s_pWidget = NULL;

ezGeneralWidget::ezGeneralWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

  ResetStats();
}

void ezGeneralWidget::ResetStats()
{
  m_bUpdateSubsystems = true;
  m_Subsystems.Clear();
}

void ezGeneralWidget::UpdateStats()
{
  if (!ezTelemetry::IsConnectedToServer())
  {
    LabelStatus->setText("<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#ff0000;\">Not Connected</span></p>");
    LabelServer->setText("<p>Server: N/A</p>");
    LabelPing->setText("<p>Ping: N/A</p>");
  }
  else
  {
    LabelStatus->setText("<p><span style=\" font-weight:600;\">Status: </span><span style=\" font-weight:600; color:#00aa00;\">Connected</span></p>");
    LabelServer->setText(QString::fromUtf8("<p>Server: %1 (%2)</p>").arg(ezTelemetry::GetServerName()).arg(ezTelemetry::GetServerIP()));
    LabelPing->setText(QString::fromUtf8("<p>Ping: %1ms</p>").arg((ezUInt32) ezTelemetry::GetPingToServer().GetMilliSeconds()));
  }

  UpdateSubSystems();
}

void ezGeneralWidget::UpdateSubSystems()
{
  if (!m_bUpdateSubsystems)
    return;

  m_bUpdateSubsystems = false;

  TableSubsystems->blockSignals(true);
  TableSubsystems->clear();

  TableSubsystems->setRowCount(m_Subsystems.GetCount());
  TableSubsystems->setColumnCount(4);

  QStringList Headers;
  Headers.append(" SubSystem ");
  Headers.append(" Plugin ");
  Headers.append(" Startup Done ");
  Headers.append(" Dependencies ");

  TableSubsystems->setHorizontalHeaderLabels(Headers);

  {
    ezStringBuilder sTemp;
    ezInt32 iRow = 0;

    for (ezMap<ezString, SubsystemData>::Iterator it = m_Subsystems.GetIterator(); it.IsValid(); ++it)
    {
      const SubsystemData& ssd = it.Value();

      sTemp.Format("  %s  ", it.Key().GetData());
      TableSubsystems->setCellWidget(iRow, 0, new QLabel(sTemp.GetData()));

      sTemp.Format("  %s  ", ssd.m_sPlugin.GetData());
      TableSubsystems->setCellWidget(iRow, 1, new QLabel(sTemp.GetData()));

      if (ssd.m_bStartupDone[ezStartupStage::Engine])
        TableSubsystems->setCellWidget(iRow, 2, new QLabel("<p><span style=\"font-weight:600; color:#00aa00;\">  Engine  </span></p>"));
      else if (ssd.m_bStartupDone[ezStartupStage::Core])
        TableSubsystems->setCellWidget(iRow, 2, new QLabel("<p><span style=\"font-weight:600; color:#5555ff;\">  Core  </span></p>"));
      else if (ssd.m_bStartupDone[ezStartupStage::Base])
        TableSubsystems->setCellWidget(iRow, 2, new QLabel("<p><span style=\"font-weight:600; color:#cece00;\">  Base  </span></p>"));
      else
        TableSubsystems->setCellWidget(iRow, 2, new QLabel("<p><span style=\"font-weight:600; color:#ff0000;\">Not Initialized</span></p>"));

      ((QLabel*) TableSubsystems->cellWidget(iRow, 2))->setAlignment(Qt::AlignHCenter);

      sTemp.Format("  %s  ", ssd.m_sDependencies.GetData());
      TableSubsystems->setCellWidget(iRow, 3, new QLabel(sTemp.GetData()));

      ++iRow;
    }
  }

  TableSubsystems->resizeColumnsToContents();

  TableSubsystems->blockSignals(false);
}

void ezGeneralWidget::ProcessTelemetry_General(void* pPassThrough)
{
  ezGeneralWidget* pWindow = (ezGeneralWidget*) pPassThrough;

  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage('APP', Msg) == EZ_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
    case 'ASRT':
      {
        ezString sSourceFile, sFunction, sExpression, sMessage;
        ezUInt32 uiLine = 0;

        Msg.GetReader() >> sSourceFile;
        Msg.GetReader() >> uiLine;
        Msg.GetReader() >> sFunction;
        Msg.GetReader() >> sExpression;
        Msg.GetReader() >> sMessage;

        ezStringBuilder sOut;

        ezMainWindow::s_pWidget->Log("");
        ezMainWindow::s_pWidget->Log("<<< Application Assertion >>>");
        ezMainWindow::s_pWidget->Log("");

        sOut.Format("    Expression: '%s'", sExpression.GetData());
        ezMainWindow::s_pWidget->Log(sOut.GetData());
        ezMainWindow::s_pWidget->Log("");

        sOut.Format("    Message: '%s'", sMessage.GetData());
        ezMainWindow::s_pWidget->Log(sOut.GetData());
        ezMainWindow::s_pWidget->Log("");

        sOut.Format("   File: '%s'", sSourceFile.GetData());
        ezMainWindow::s_pWidget->Log(sOut.GetData());

        sOut.Format("   Line: %i", uiLine);
        ezMainWindow::s_pWidget->Log(sOut.GetData());

        sOut.Format("   In Function: '%s'", sFunction.GetData());
        ezMainWindow::s_pWidget->Log(sOut.GetData());

        ezMainWindow::s_pWidget->Log("");

        ezMainWindow::s_pWidget->Log(">>> Application Assertion <<<");
        ezMainWindow::s_pWidget->Log("");
      }
      break;
    case 'DATA':
      {
        ezString sPlatform;
        ezUInt32 uiCoreCount = 0;
        ezUInt64 uiMemorySize = 0;
        bool bIs64Bit = false;

        Msg.GetReader() >> sPlatform;
        Msg.GetReader() >> uiCoreCount;
        Msg.GetReader() >> uiMemorySize;
        Msg.GetReader() >> bIs64Bit;

        ezStringBuilder sInfo;
        sInfo.Format("%s %s, %i Cores, %.1f GB RAM", sPlatform.GetData(), bIs64Bit ? "64 Bit" : "32 Bit", uiCoreCount, uiMemorySize / 1024.0f / 1024.0f / 1024.0f);

        pWindow->LabelAppName->setText(sInfo.GetData());
      }
      break;
    }
  }

  while (ezTelemetry::RetrieveMessage('STRT', Msg) == EZ_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
    case 'SYST':
      {
        ezString sGroup, sSystem;

        Msg.GetReader() >> sGroup;
        Msg.GetReader() >> sSystem;

        ezStringBuilder sFinalName = sGroup.GetData();
        sFinalName.Append("::");
        sFinalName.Append(sSystem.GetData());

        SubsystemData& ssd = s_pWidget->m_Subsystems[sFinalName.GetData()];

        Msg.GetReader() >> ssd.m_sPlugin;

        for (ezUInt32 i = 0; i < ezStartupStage::ENUM_COUNT; ++i)
          Msg.GetReader() >> ssd.m_bStartupDone[i];

        ezUInt8 uiDependencies = 0;
        Msg.GetReader() >> uiDependencies;

        ezStringBuilder sAllDeps;

        ezString sDep;
        for (ezUInt8 i = 0; i < uiDependencies; ++i)
        {
          Msg.GetReader() >> sDep;

          if (sAllDeps.IsEmpty())
            sAllDeps = sDep.GetData();
          else
          {
            sAllDeps.Append(" | ");
            sAllDeps.Append(sDep.GetData());
          }
        }

        ssd.m_sDependencies = sAllDeps.GetData();

        s_pWidget->m_bUpdateSubsystems = true;
      }
      break;
    }
  }
}

void ezGeneralWidget::on_ButtonConnect_clicked()
{
  bool bOk = false;
  QString sRes = QInputDialog::getText(this, "Input Server Name or IP Address", "", QLineEdit::Normal, "", &bOk);

  if (!bOk)
    return;

  if (ezTelemetry::ConnectToServer(sRes.toUtf8().data()) == EZ_SUCCESS)
  {
  }
}


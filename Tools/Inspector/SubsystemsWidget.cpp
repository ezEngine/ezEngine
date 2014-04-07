#include <PCH.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <MainWindow.moc.h>

ezSubsystemsWidget* ezSubsystemsWidget::s_pWidget = nullptr;

ezSubsystemsWidget::ezSubsystemsWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

  ResetStats();
}

void ezSubsystemsWidget::ResetStats()
{
  m_bUpdateSubsystems = true;
  m_Subsystems.Clear();
}


void ezSubsystemsWidget::UpdateStats()
{
  UpdateSubSystems();
}

void ezSubsystemsWidget::UpdateSubSystems()
{
  if (!m_bUpdateSubsystems)
    return;

  m_bUpdateSubsystems = false;

  TableSubsystems->blockSignals(true);
  TableSubsystems->clear();

  TableSubsystems->setRowCount(m_Subsystems.GetCount());

  QStringList Headers;
  Headers.append("");
  Headers.append(" SubSystem ");
  Headers.append(" Plugin ");
  Headers.append(" Startup Done ");
  Headers.append(" Dependencies ");

  TableSubsystems->setColumnCount(Headers.size());

  TableSubsystems->setHorizontalHeaderLabels(Headers);

  {
    ezStringBuilder sTemp;
    ezInt32 iRow = 0;

    for (ezMap<ezString, SubsystemData>::Iterator it = m_Subsystems.GetIterator(); it.IsValid(); ++it)
    {
      const SubsystemData& ssd = it.Value();

      QLabel* pIcon = new QLabel();
      pIcon->setPixmap(QPixmap(":/Icons/Icons/Subsystem.png"));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TableSubsystems->setCellWidget(iRow, 0, pIcon);

      sTemp.Format("  %s  ", it.Key().GetData());
      TableSubsystems->setCellWidget(iRow, 1, new QLabel(sTemp.GetData()));

      sTemp.Format("  %s  ", ssd.m_sPlugin.GetData());
      TableSubsystems->setCellWidget(iRow, 2, new QLabel(sTemp.GetData()));

      if (ssd.m_bStartupDone[ezStartupStage::Engine])
        TableSubsystems->setCellWidget(iRow, 3, new QLabel("<p><span style=\"font-weight:600; color:#00aa00;\">  Engine  </span></p>"));
      else if (ssd.m_bStartupDone[ezStartupStage::Core])
        TableSubsystems->setCellWidget(iRow, 3, new QLabel("<p><span style=\"font-weight:600; color:#5555ff;\">  Core  </span></p>"));
      else if (ssd.m_bStartupDone[ezStartupStage::Base])
        TableSubsystems->setCellWidget(iRow, 3, new QLabel("<p><span style=\"font-weight:600; color:#cece00;\">  Base  </span></p>"));
      else
        TableSubsystems->setCellWidget(iRow, 3, new QLabel("<p><span style=\"font-weight:600; color:#ff0000;\">Not Initialized</span></p>"));

      ((QLabel*) TableSubsystems->cellWidget(iRow, 3))->setAlignment(Qt::AlignHCenter);

      sTemp.Format("  %s  ", ssd.m_sDependencies.GetData());
      TableSubsystems->setCellWidget(iRow, 4, new QLabel(sTemp.GetData()));

      ++iRow;
    }
  }

  TableSubsystems->resizeColumnsToContents();

  TableSubsystems->blockSignals(false);
}

void ezSubsystemsWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage('STRT', Msg) == EZ_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
    case 'CLR':
      {
        s_pWidget->m_Subsystems.Clear();
        s_pWidget->m_bUpdateSubsystems = true;
      }
      break;

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



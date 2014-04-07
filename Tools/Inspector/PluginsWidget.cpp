#include <PCH.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <MainWindow.moc.h>

ezPluginsWidget* ezPluginsWidget::s_pWidget = nullptr;

ezPluginsWidget::ezPluginsWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

  ResetStats();
}

void ezPluginsWidget::ResetStats()
{
  m_bUpdatePlugins = true;
  m_Plugins.Clear();
}


void ezPluginsWidget::UpdateStats()
{
  UpdatePlugins();
}

void ezPluginsWidget::UpdatePlugins()
{
  if (!m_bUpdatePlugins)
    return;

  m_bUpdatePlugins = false;

  TablePlugins->blockSignals(true);
  TablePlugins->clear();

  TablePlugins->setRowCount(m_Plugins.GetCount());

  QStringList Headers;
  Headers.append("");
  Headers.append(" Plugin ");
  Headers.append(" Reloadable ");
  Headers.append(" Dependencies ");

  TablePlugins->setColumnCount(Headers.size());

  TablePlugins->setHorizontalHeaderLabels(Headers);

  {
    ezStringBuilder sTemp;
    ezInt32 iRow = 0;

    for (ezMap<ezString, PluginsData>::Iterator it = m_Plugins.GetIterator(); it.IsValid(); ++it)
    {
      const PluginsData& ssd = it.Value();

      QLabel* pIcon = new QLabel();
      pIcon->setPixmap(QPixmap(":/Icons/Icons/Plugin.png"));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TablePlugins->setCellWidget(iRow, 0, pIcon);

      sTemp.Format("  %s  ", it.Key().GetData());
      TablePlugins->setCellWidget(iRow, 1, new QLabel(sTemp.GetData()));
      
      if (it.Value().m_bReloadable)
        TablePlugins->setCellWidget(iRow, 2, new QLabel("<p><span style=\"font-weight:600; color:#00aa00;\">  Yes  </span></p>"));
      else
        TablePlugins->setCellWidget(iRow, 2, new QLabel("<p><span style=\"font-weight:600; color:#ffaa00;\">  No  </span></p>"));

      ((QLabel*) TablePlugins->cellWidget(iRow, 2))->setAlignment(Qt::AlignHCenter);

      TablePlugins->setCellWidget(iRow, 3, new QLabel(it.Value().m_sDependencies.GetData()));

      ++iRow;
    }
  }

  TablePlugins->resizeColumnsToContents();

  TablePlugins->blockSignals(false);
}

void ezPluginsWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage('PLUG', Msg) == EZ_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
    case 'CLR':
      {
        s_pWidget->m_Plugins.Clear();
        s_pWidget->m_bUpdatePlugins = true;
      }
      break;

    case 'DATA':
      {
        ezString sName;

        Msg.GetReader() >> sName;

        PluginsData& pd = s_pWidget->m_Plugins[sName.GetData()];
        Msg.GetReader() >> pd.m_bReloadable;

        Msg.GetReader() >> pd.m_sDependencies;

        s_pWidget->m_bUpdatePlugins = true;
      }
      break;
    }
  }
}



#include <PCH.h>
#include <Inspector/GlobalEventsWidget.moc.h>
#include <qlistwidget.h>
#include <Foundation/Communication/Telemetry.h>
#include <MainWindow.moc.h>

ezGlobalEventsWidget* ezGlobalEventsWidget::s_pWidget = nullptr;

ezGlobalEventsWidget::ezGlobalEventsWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

  ResetStats();
}

void ezGlobalEventsWidget::ResetStats()
{
  m_Events.Clear();
  TableEvents->clear();

  {
    QStringList Headers;
    Headers.append(" Event ");
    Headers.append(" Times Fired ");
    Headers.append(" # Handlers ");
    Headers.append(" # Handlers Once ");

    TableEvents->setColumnCount(Headers.size());

    TableEvents->setHorizontalHeaderLabels(Headers);
    TableEvents->horizontalHeader()->show();
  }
}

void ezGlobalEventsWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage msg;

  bool bUpdateTable = false;
  bool bFillTable = false;

  while (ezTelemetry::RetrieveMessage('EVNT', msg) == EZ_SUCCESS)
  {
    if (msg.GetMessageID() == 'CLR')
    {
      s_pWidget->m_Events.Clear();
    }

    if (msg.GetMessageID() == 'DATA')
    {
      ezString sName;
      msg.GetReader() >> sName;

      GlobalEventsData& sd = s_pWidget->m_Events[sName];

      msg.GetReader() >> sd.m_uiTimesFired;
      msg.GetReader() >> sd.m_uiNumHandlers;
      msg.GetReader() >> sd.m_uiNumHandlersOnce;

      if (sd.m_iTableRow == -1)
        bUpdateTable = true;

      bFillTable = true;
    }
  }

  if (bUpdateTable)
    s_pWidget->UpdateTable(true);
  else
  if (bFillTable)
    s_pWidget->UpdateTable(false);

}

void ezGlobalEventsWidget::UpdateTable(bool bRecreate)
{
  TableEvents->blockSignals(true);

  if (bRecreate)
  {
    TableEvents->clear();
    TableEvents->setRowCount(m_Events.GetCount());

    QStringList Headers;
    Headers.append(" ");
    Headers.append(" Event ");
    Headers.append(" Times Fired ");
    Headers.append(" # Handlers ");
    Headers.append(" # Handlers Once ");

    TableEvents->setColumnCount(Headers.size());

    TableEvents->setHorizontalHeaderLabels(Headers);
    TableEvents->horizontalHeader()->show();

    ezStringBuilder sTemp;

    ezInt32 iRow = 0;
    for (ezMap<ezString, GlobalEventsData>::Iterator it = m_Events.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_iTableRow = iRow;

      QLabel* pIcon = new QLabel();
      pIcon->setPixmap(QPixmap(":/Icons/Icons/GlobalEvent.png"));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TableEvents->setCellWidget(iRow, 0, pIcon);

      sTemp.Format("  %s  ", it.Key().GetData());
      TableEvents->setCellWidget(iRow, 1, new QLabel(sTemp.GetData())); // Event

      sTemp.Format("  %i  ", it.Value().m_uiTimesFired);
      TableEvents->setCellWidget(iRow, 2, new QLabel(sTemp.GetData()));

      sTemp.Format("  %i  ", it.Value().m_uiNumHandlers);
      TableEvents->setCellWidget(iRow, 3, new QLabel(sTemp.GetData()));

      sTemp.Format("  %i  ", it.Value().m_uiNumHandlersOnce);
      TableEvents->setCellWidget(iRow, 4, new QLabel(sTemp.GetData()));

      ++iRow;
    }

    TableEvents->resizeColumnsToContents();
  }
  else
  {
    ezStringBuilder sTemp;

    ezInt32 iRow = 0;
    for (ezMap<ezString, GlobalEventsData>::Iterator it = m_Events.GetIterator(); it.IsValid(); ++it)
    {
      sTemp.Format("  %i  ", it.Value().m_uiTimesFired);
      ((QLabel*) TableEvents->cellWidget(iRow, 2))->setText(sTemp.GetData());

      sTemp.Format("  %i  ", it.Value().m_uiNumHandlers);
      ((QLabel*) TableEvents->cellWidget(iRow, 3))->setText(sTemp.GetData());

      sTemp.Format("  %i  ", it.Value().m_uiNumHandlersOnce);
      ((QLabel*) TableEvents->cellWidget(iRow, 4))->setText(sTemp.GetData());

      ++iRow;
    }
  }

  TableEvents->blockSignals(false);
}


#include <Inspector/InputWidget.moc.h>
#include <qlistwidget.h>
#include <Foundation/Communication/Telemetry.h>
#include <MainWindow.moc.h>

ezInputWidget* ezInputWidget::s_pWidget = NULL;

ezInputWidget::ezInputWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

  ResetStats();
}

void ezInputWidget::ResetStats()
{
  m_Inputslots.Clear();
  TableInputSlots->clear();
}

void ezInputWidget::UpdateStats()
{
}

void ezInputWidget::ProcessTelemetry_Input(void* pPassThrough)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage msg;

  bool bUpdateTable = false;
  bool bFillTable = false;

  while (ezTelemetry::RetrieveMessage('INPT', msg) == EZ_SUCCESS)
  {
    if (msg.GetMessageID() == 'SLOT')
    {
      ezString sSlotName;
      msg.GetReader() >> sSlotName;

      SlotData& sd = s_pWidget->m_Inputslots[sSlotName];

      msg.GetReader() >> sd.m_uiSlotFlags;

      ezUInt8 uiKeyState = 0;
      msg.GetReader() >> uiKeyState;
      sd.m_KeyState = (ezKeyState::Enum) uiKeyState;

      msg.GetReader() >> sd.m_fValue;
      msg.GetReader() >> sd.m_fDeadZone;

      if (sd.m_iTableRow == -1)
        bUpdateTable = true;

      bFillTable = true;
    }
  }

  if (bUpdateTable)
    s_pWidget->UpdateSlotTable(true);
  else
  if (bFillTable)
    s_pWidget->UpdateSlotTable(false);
}

void ezInputWidget::UpdateSlotTable(bool bRecreate)
{
  TableInputSlots->blockSignals(true);

  if (bRecreate)
  {
    TableInputSlots->clear();
    TableInputSlots->setRowCount(m_Inputslots.GetCount());
    TableInputSlots->setColumnCount(5);

    QStringList Headers;
    Headers.append("Slot");
    Headers.append("State");
    Headers.append("Value");
    Headers.append("Dead Zone");
    Headers.append("Flags (Binary)");

    TableInputSlots->setHorizontalHeaderLabels(Headers);

    ezInt32 iRow = 0;
    for (ezMap<ezString, SlotData>::Iterator it = m_Inputslots.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_iTableRow = iRow;

      TableInputSlots->setCellWidget(iRow, 0, new QLabel(it.Key().GetData()));
      TableInputSlots->setCellWidget(iRow, 1, new QLabel(""));
      TableInputSlots->setCellWidget(iRow, 2, new QLabel(""));
      TableInputSlots->setCellWidget(iRow, 3, new QLabel(""));
      TableInputSlots->setCellWidget(iRow, 4, new QLabel(""));

      {
        ezStringBuilder sFlags;
        sFlags.Format("%16b", it.Value().m_uiSlotFlags);

        QLabel* pFlags = (QLabel*) TableInputSlots->cellWidget(iRow, 4);
        pFlags->setAlignment(Qt::AlignRight);
        pFlags->setText(sFlags.GetData());
      }

      ++iRow;
    }

    TableInputSlots->resizeColumnToContents(0);
  }
  
  {
    ezInt32 iRow = 0;
    for (ezMap<ezString, SlotData>::Iterator it = m_Inputslots.GetIterator(); it.IsValid(); ++it)
    {
      QLabel* pState = (QLabel*) TableInputSlots->cellWidget(iRow, 1);
      pState->setAlignment(Qt::AlignHCenter);

      switch (it.Value().m_KeyState)
      {
      case ezKeyState::Down:
        pState->setText("Down");
        break;
      case ezKeyState::Pressed:
        pState->setText("Pressed");
        break;
      case ezKeyState::Released:
        pState->setText("Released");
        break;
      case ezKeyState::Up:
        pState->setText("");
        break;
      }

      {
        QLabel* pValue = (QLabel*) TableInputSlots->cellWidget(iRow, 2);
        pValue->setAlignment(Qt::AlignHCenter);

        if (it.Value().m_fValue == 0.0f)
          pValue->setText("");
        else
          pValue->setText(QString::number(it.Value().m_fValue, 'f', 2));
      }

      {
        QLabel* pDeadZone = (QLabel*) TableInputSlots->cellWidget(iRow, 3);
        pDeadZone->setAlignment(Qt::AlignHCenter);

        if (it.Value().m_fDeadZone == 0.0f)
          pDeadZone->setText("");
        else
          pDeadZone->setText(QString::number(it.Value().m_fDeadZone, 'f', 2));
      }

      ++iRow;
    }
  }

  TableInputSlots->blockSignals(false);
}

void ezInputWidget::on_ButtonClear_clicked()
{
  ResetStats();
}


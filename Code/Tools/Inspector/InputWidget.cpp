#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <qlistwidget.h>

ezQtInputWidget* ezQtInputWidget::s_pWidget = nullptr;

ezQtInputWidget::ezQtInputWidget(QWidget* pParent)
  : ads::CDockWidget("Input Widget", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(InputWidgetFrame);

  setIcon(QIcon(":/Icons/Icons/InputActions.svg"));

  ResetStats();
}

void ezQtInputWidget::ResetStats()
{
  ClearSlots();
  ClearActions();
}

void ezQtInputWidget::ClearSlots()
{
  m_InputSlots.Clear();
  TableInputSlots->clear();

  {
    QStringList Headers;
    Headers.append("");
    Headers.append(" Slot ");
    Headers.append(" State ");
    Headers.append(" Value ");
    Headers.append(" Dead Zone ");
    Headers.append(" Flags (Binary) ");

    TableInputSlots->setColumnCount(static_cast<int>(Headers.size()));

    TableInputSlots->setHorizontalHeaderLabels(Headers);
    TableInputSlots->horizontalHeader()->show();
  }
}

void ezQtInputWidget::ClearActions()
{
  m_InputActions.Clear();
  TableInputActions->clear();

  {
    QStringList Headers;
    Headers.append("");
    Headers.append(" Action ");
    Headers.append(" State ");
    Headers.append(" Value ");

    for (ezInt32 slot = 0; slot < ezInputActionConfig::MaxInputSlotAlternatives; ++slot)
      Headers.append(QString(" Slot %1 ").arg(slot + 1));

    TableInputActions->setColumnCount(static_cast<int>(Headers.size()));

    TableInputActions->setHorizontalHeaderLabels(Headers);
    TableInputActions->horizontalHeader()->show();
  }
}

void ezQtInputWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage msg;

  bool bUpdateSlotTable = false;
  bool bFillSlotTable = false;
  bool bUpdateActionTable = false;
  bool bFillActionTable = false;

  while (ezTelemetry::RetrieveMessage('INPT', msg) == EZ_SUCCESS)
  {
    if (msg.GetMessageID() == 'SLOT')
    {
      ezString sSlotName;
      msg.GetReader() >> sSlotName;

      SlotData& sd = s_pWidget->m_InputSlots[sSlotName];

      msg.GetReader() >> sd.m_uiSlotFlags;

      ezUInt8 uiKeyState = 0;
      msg.GetReader() >> uiKeyState;
      sd.m_KeyState = (ezKeyState::Enum)uiKeyState;

      msg.GetReader() >> sd.m_fValue;
      msg.GetReader() >> sd.m_fDeadZone;

      if (sd.m_iTableRow == -1)
        bUpdateSlotTable = true;

      bFillSlotTable = true;
    }

    if (msg.GetMessageID() == 'ACTN')
    {
      ezString sInputSetName;
      msg.GetReader() >> sInputSetName;

      ezString sActionName;
      msg.GetReader() >> sActionName;

      ezStringBuilder sFinalName = sInputSetName.GetData();
      sFinalName.Append("::");
      sFinalName.Append(sActionName.GetData());

      ActionData& sd = s_pWidget->m_InputActions[sFinalName.GetData()];

      ezUInt8 uiKeyState = 0;
      msg.GetReader() >> uiKeyState;
      sd.m_KeyState = (ezKeyState::Enum)uiKeyState;

      msg.GetReader() >> sd.m_fValue;
      msg.GetReader() >> sd.m_bUseTimeScaling;

      for (ezUInt32 i = 0; i < ezInputActionConfig::MaxInputSlotAlternatives; ++i)
      {
        msg.GetReader() >> sd.m_sTrigger[i];
        msg.GetReader() >> sd.m_fTriggerScaling[i];
      }

      if (sd.m_iTableRow == -1)
        bUpdateActionTable = true;

      bFillActionTable = true;
    }
  }

  if (bUpdateSlotTable)
    s_pWidget->UpdateSlotTable(true);
  else if (bFillSlotTable)
    s_pWidget->UpdateSlotTable(false);

  if (bUpdateActionTable)
    s_pWidget->UpdateActionTable(true);
  else if (bFillActionTable)
    s_pWidget->UpdateActionTable(false);
}

void ezQtInputWidget::UpdateSlotTable(bool bRecreate)
{
  ezQtScopedUpdatesDisabled _1(TableInputSlots);

  if (bRecreate)
  {
    TableInputSlots->clear();
    TableInputSlots->setRowCount(m_InputSlots.GetCount());

    QStringList Headers;
    Headers.append("");
    Headers.append(" Slot ");
    Headers.append(" State ");
    Headers.append(" Value ");
    Headers.append(" Dead Zone ");
    Headers.append(" Flags (Binary) ");

    TableInputSlots->setColumnCount(static_cast<int>(Headers.size()));

    TableInputSlots->setHorizontalHeaderLabels(Headers);
    TableInputSlots->horizontalHeader()->show();

    ezStringBuilder sTemp;

    ezInt32 iRow = 0;
    for (ezMap<ezString, SlotData>::Iterator it = m_InputSlots.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_iTableRow = iRow;

      sTemp.SetFormat("  {0}  ", it.Key());

      QLabel* pIcon = new QLabel();
      QIcon icon = ezQtUiServices::GetCachedIconResource(":/Icons/Icons/InputSlots.svg");
      pIcon->setPixmap(icon.pixmap(QSize(24, 24)));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TableInputSlots->setCellWidget(iRow, 0, pIcon);

      TableInputSlots->setCellWidget(iRow, 1, new QLabel(sTemp.GetData()));
      TableInputSlots->setCellWidget(iRow, 2, new QLabel("????????????"));
      TableInputSlots->setCellWidget(iRow, 3, new QLabel("????????"));
      TableInputSlots->setCellWidget(iRow, 4, new QLabel("????????"));
      TableInputSlots->setCellWidget(iRow, 5, new QLabel("????????????????"));

      const ezUInt32 uiFlags = it.Value().m_uiSlotFlags;

      // Flags
      {
        ezStringBuilder sFlags;
        sFlags.SetPrintf("  %16b  ", uiFlags);

        QLabel* pFlags = (QLabel*)TableInputSlots->cellWidget(iRow, 5);
        pFlags->setAlignment(Qt::AlignRight);
        pFlags->setText(sFlags.GetData());
      }

      // Flags Tooltip
      {
        // in VS 2012 at least the snprintf fails when "yes" and "no" are passed directly, instead of as const char* variables
        const char* szYes = "<b>yes</b>";
        const char* szNo = "no";

        ezStringBuilder tt("<p>");
        tt.AppendFormat("ReportsRelativeValues: {0}<br>", (uiFlags & ezInputSlotFlags::ReportsRelativeValues) ? szYes : szNo);
        tt.AppendFormat("ValueBinaryZeroOrOne: {0}<br>", (uiFlags & ezInputSlotFlags::ValueBinaryZeroOrOne) ? szYes : szNo);
        tt.AppendFormat("ValueRangeZeroToOne: {0}<br>", (uiFlags & ezInputSlotFlags::ValueRangeZeroToOne) ? szYes : szNo);
        tt.AppendFormat("ValueRangeZeroToInf: {0}<br>", (uiFlags & ezInputSlotFlags::ValueRangeZeroToInf) ? szYes : szNo);
        tt.AppendFormat("Pressable: {0}<br>", (uiFlags & ezInputSlotFlags::Pressable) ? szYes : szNo);
        tt.AppendFormat("Holdable: {0}<br>", (uiFlags & ezInputSlotFlags::Holdable) ? szYes : szNo);
        tt.AppendFormat("HalfAxis: {0}<br>", (uiFlags & ezInputSlotFlags::HalfAxis) ? szYes : szNo);
        tt.AppendFormat("FullAxis: {0}<br>", (uiFlags & ezInputSlotFlags::FullAxis) ? szYes : szNo);
        tt.AppendFormat("RequiresDeadZone: {0}<br>", (uiFlags & ezInputSlotFlags::RequiresDeadZone) ? szYes : szNo);
        tt.AppendFormat("ValuesAreNonContinuous: {0}<br>", (uiFlags & ezInputSlotFlags::ValuesAreNonContinuous) ? szYes : szNo);
        tt.AppendFormat("ActivationDependsOnOthers: {0}<br>", (uiFlags & ezInputSlotFlags::ActivationDependsOnOthers) ? szYes : szNo);
        tt.AppendFormat("NeverTimeScale: {0}<br>", (uiFlags & ezInputSlotFlags::NeverTimeScale) ? szYes : szNo);
        tt.Append("</p>");

        TableInputSlots->cellWidget(iRow, 5)->setToolTip(tt.GetData());
      }

      ++iRow;
    }

    TableInputSlots->resizeColumnsToContents();
  }

  {
    ezStringBuilder sTemp;

    ezInt32 iRow = 0;
    for (ezMap<ezString, SlotData>::Iterator it = m_InputSlots.GetIterator(); it.IsValid(); ++it)
    {
      QLabel* pState = (QLabel*)TableInputSlots->cellWidget(iRow, 2);
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

      // Value
      {
        QLabel* pValue = (QLabel*)TableInputSlots->cellWidget(iRow, 3);
        pValue->setAlignment(Qt::AlignHCenter);

        if (it.Value().m_fValue == 0.0f)
          pValue->setText("");
        else
        {
          sTemp.SetFormat(" {0} ", ezArgF(it.Value().m_fValue, 4));
          pValue->setText(sTemp.GetData());
        }
      }

      // Dead-zone
      {
        QLabel* pDeadZone = (QLabel*)TableInputSlots->cellWidget(iRow, 4);
        pDeadZone->setAlignment(Qt::AlignHCenter);

        if (it.Value().m_fDeadZone == 0.0f)
          pDeadZone->setText("");
        else
          pDeadZone->setText(QString::number(it.Value().m_fDeadZone, 'f', 2));
      }

      ++iRow;
    }
  }
}

void ezQtInputWidget::UpdateActionTable(bool bRecreate)
{
  ezQtScopedUpdatesDisabled _1(TableInputActions);

  if (bRecreate)
  {
    TableInputActions->clear();
    TableInputActions->setRowCount(m_InputActions.GetCount());

    QStringList Headers;
    Headers.append("");
    Headers.append(" Action ");
    Headers.append(" State ");
    Headers.append(" Value ");

    for (ezInt32 slot = 0; slot < ezInputActionConfig::MaxInputSlotAlternatives; ++slot)
      Headers.append(QString(" Slot %1 ").arg(slot + 1));

    TableInputActions->setColumnCount(static_cast<int>(Headers.size()));

    TableInputActions->setHorizontalHeaderLabels(Headers);
    TableInputActions->horizontalHeader()->show();

    ezStringBuilder sTemp;

    ezInt32 iRow = 0;
    for (ezMap<ezString, ActionData>::Iterator it = m_InputActions.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_iTableRow = iRow;

      sTemp.SetFormat("  {0}  ", it.Key());

      QLabel* pIcon = new QLabel();
      QIcon icon = ezQtUiServices::GetCachedIconResource(":/Icons/Icons/InputActions.svg");
      pIcon->setPixmap(icon.pixmap(QSize(24, 24)));
      pIcon->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      TableInputActions->setCellWidget(iRow, 0, pIcon);

      TableInputActions->setCellWidget(iRow, 1, new QLabel(sTemp.GetData()));
      TableInputActions->setCellWidget(iRow, 2, new QLabel("????????????"));
      TableInputActions->setCellWidget(iRow, 3, new QLabel("????????????????????????"));
      TableInputActions->setCellWidget(iRow, 4, new QLabel(""));
      TableInputActions->setCellWidget(iRow, 5, new QLabel(""));
      TableInputActions->setCellWidget(iRow, 6, new QLabel(""));

      // Trigger Slots

      for (ezInt32 slot = 0; slot < ezInputActionConfig::MaxInputSlotAlternatives; ++slot)
      {
        if (it.Value().m_sTrigger[slot].IsEmpty())
          sTemp = "  ";
        else
          sTemp.SetFormat("  [Scale: {0}] {1}  ", ezArgF(it.Value().m_fTriggerScaling[slot], 2), it.Value().m_sTrigger[slot]);

        QLabel* pValue = (QLabel*)TableInputActions->cellWidget(iRow, 4 + slot);
        pValue->setText(sTemp.GetData());
      }

      ++iRow;
    }

    TableInputActions->resizeColumnsToContents();
  }

  {
    ezStringBuilder sTemp;

    ezInt32 iRow = 0;
    for (ezMap<ezString, ActionData>::Iterator it = m_InputActions.GetIterator(); it.IsValid(); ++it)
    {
      QLabel* pState = (QLabel*)TableInputActions->cellWidget(iRow, 2);
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

      // Value
      {
        QLabel* pValue = (QLabel*)TableInputActions->cellWidget(iRow, 3);
        pValue->setAlignment(Qt::AlignHCenter);

        if (it.Value().m_fValue == 0.0f)
          pValue->setText("");
        else
        {
          if (it.Value().m_bUseTimeScaling)
            sTemp.SetFormat(" {0} (Time-Scaled) ", ezArgF(it.Value().m_fValue, 4));
          else
            sTemp.SetFormat(" {0} (Absolute) ", ezArgF(it.Value().m_fValue, 4));

          pValue->setText(sTemp.GetData());
        }
      }


      ++iRow;
    }
  }
}

void ezQtInputWidget::on_ButtonClearSlots_clicked()
{
  ClearSlots();
}

void ezQtInputWidget::on_ButtonClearActions_clicked()
{
  ClearActions();
}

#include <PCH.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <MainWindow.moc.h>
#include <QTableWidget>
#include <QLabel>

ezDataWidget* ezDataWidget::s_pWidget = NULL;

ezDataWidget::ezDataWidget(QWidget* parent) : QDockWidget(parent)
{
  s_pWidget = this;

  setupUi (this);

  ResetStats();
}

void ezDataWidget::ResetStats()
{
  for (auto it = m_Data.GetIterator(); it.IsValid(); ++it)
    delete it.Value().m_pItem;

  m_Data.Clear();
  DataTable->clear();
  DataTable->setRowCount(0);

  {
    QStringList Headers;
    Headers.append("Transfer");

    DataTable->setColumnCount(Headers.size());

    DataTable->setHorizontalHeaderLabels(Headers);
    DataTable->horizontalHeader()->show();
  }
}

void ezDataWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage msg;
  
  while (ezTelemetry::RetrieveMessage('TRAN', msg) == EZ_SUCCESS)
  {
    if (msg.GetMessageID() == 'CLR')
    {
      s_pWidget->ResetStats();
    }

    if (msg.GetMessageID() == 'ENBL')
    {
      ezString sName;
      msg.GetReader() >> sName;

      TransferData& td = s_pWidget->m_Data[sName];

      if (td.m_iRow == -1)
      {
        const ezInt32 iRows = s_pWidget->DataTable->rowCount();

        td.m_iRow = iRows;

        s_pWidget->DataTable->setRowCount(iRows + 1);

        if (s_pWidget->DataTable->item(iRows, 0) == NULL)
        {
          td.m_pItem = new QTableWidgetItem(sName.GetData());

          s_pWidget->DataTable->setItem(iRows, 0, td.m_pItem);
        }
      }
    }

    if (msg.GetMessageID() == 'DSBL')
    {
      ezString sName;
      msg.GetReader() >> sName;

      auto it = s_pWidget->m_Data.Find(sName);

      if (it.IsValid())
      {
        if (it.Value().m_iRow != -1)
          s_pWidget->DataTable->removeRow(it.Value().m_iRow);

        s_pWidget->m_Data.Erase(it);
      }
    }

    if (msg.GetMessageID() == 'DATA')
    {
      ezString sBelongsTo, sName, sMimeType;

      msg.GetReader() >> sBelongsTo;
      msg.GetReader() >> sName;
      msg.GetReader() >> sMimeType;

      auto Transfer = s_pWidget->m_Data.Find(sBelongsTo);

      if (Transfer.IsValid())
      {
        TransferDataObject& tdo = Transfer.Value().m_Objects[sName];
        tdo.m_sMimeType = sMimeType;
        msg.GetReader() >> tdo.m_sText;

        ezInt32 iBits;
        msg.GetReader() >> tdo.m_uiWidth;
        msg.GetReader() >> tdo.m_uiHeight;
        msg.GetReader() >> iBits;

        ezStringBuilder s;
        s.Format("Text: '%s', Image: %i * %i * %i", tdo.m_sText.GetData(), tdo.m_uiWidth, tdo.m_uiHeight, iBits);
        tdo.m_sText = s.GetData();
        
        tdo.m_Image.SetCount(tdo.m_uiWidth * tdo.m_uiHeight * iBits);
        msg.GetReader().ReadBytes(&tdo.m_Image[0], tdo.m_Image.GetCount());
      }

      s_pWidget->on_DataTable_itemSelectionChanged();
    }
  }
}

void ezDataWidget::on_DataTable_itemSelectionChanged()
{
  if (DataTable->currentRow() < 0)
    return;

  if (DataTable->item(DataTable->currentRow(), 0) == NULL)
    return;

  const ezString sName = DataTable->item(DataTable->currentRow(), 0)->text().toUtf8().data();

  auto it = m_Data.Find(sName);

  if (!it.IsValid())
    return;

  if (it.Value().m_Objects.IsEmpty())
    return;

  const auto& Data = it.Value().m_Objects.GetIterator().Value();
  LabelData->setText(Data.m_sText.GetData());

  const ezDynamicArray<ezUInt8>& Image = Data.m_Image;

  if (!Image.IsEmpty())
  {
    QImage i(&Image[0], Data.m_uiWidth, Data.m_uiHeight, QImage::Format_ARGB32);

    LabelImage->setPixmap(QPixmap::fromImage(i));
  }
}

void ezDataWidget::on_ButtonRefresh_clicked()
{
  if (DataTable->currentRow() < 0)
    return;

  const ezString sName = DataTable->item(DataTable->currentRow(), 0)->text().toUtf8().data();

  auto it = m_Data.Find(sName);

  if (!it.IsValid())
    return;

  ezTelemetryMessage msg;
  msg.SetMessageID('DTRA', 'REQ');
  msg.GetWriter() << it.Key();
  ezTelemetry::SendToServer(msg);
}
#include <PCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <MainWindow.moc.h>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QTableWidget>
#include <QTemporaryFile>
#include <QUrl>
#include <qdesktopservices.h>

ezQtDataWidget* ezQtDataWidget::s_pWidget = nullptr;

ezQtDataWidget::ezQtDataWidget(QWidget* parent)
    : QDockWidget(parent)
{
  /// \todo Improve Data Transfer UI

  s_pWidget = this;

  setupUi(this);

  ResetStats();
}

void ezQtDataWidget::ResetStats()
{
  m_Transfers.Clear();
  ComboTransfers->clear();
  ComboItems->clear();
}

void ezQtDataWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage msg;

  while (ezTelemetry::RetrieveMessage('TRAN', msg) == EZ_SUCCESS)
  {
    if (msg.GetMessageID() == ' CLR')
    {
      s_pWidget->ResetStats();
    }

    if (msg.GetMessageID() == 'ENBL')
    {
      ezString sName;
      msg.GetReader() >> sName;

      s_pWidget->ComboTransfers->addItem(sName.GetData());
    }

    if (msg.GetMessageID() == 'DSBL')
    {
      ezString sName;
      msg.GetReader() >> sName;

      auto it = s_pWidget->m_Transfers.Find(sName);

      if (it.IsValid())
      {
        ezInt32 iIndex = s_pWidget->ComboTransfers->findText(sName.GetData());

        if (iIndex >= 0)
          s_pWidget->ComboTransfers->removeItem(iIndex);
      }
    }

    if (msg.GetMessageID() == 'DATA')
    {
      ezString sBelongsTo, sName, sMimeType, sExtension;

      msg.GetReader() >> sBelongsTo;
      msg.GetReader() >> sName;
      msg.GetReader() >> sMimeType;
      msg.GetReader() >> sExtension;

      auto Transfer = s_pWidget->m_Transfers.Find(sBelongsTo);

      if (Transfer.IsValid())
      {
        TransferDataObject& tdo = Transfer.Value().m_Items[sName];
        tdo.m_sMimeType = sMimeType;
        tdo.m_sExtension = sExtension;

        ezMemoryStreamWriter Writer(&tdo.m_Storage);

        // copy the entire memory stream over and store it for later
        while (true)
        {
          ezUInt8 uiTemp[1024];
          const ezUInt32 uiRead = msg.GetReader().ReadBytes(uiTemp, 1024);

          if (uiRead == 0)
            break;

          Writer.WriteBytes(uiTemp, uiRead);
        }

        s_pWidget->on_ComboTransfers_currentIndexChanged(s_pWidget->ComboTransfers->currentIndex());
      }
    }
  }
}

void ezQtDataWidget::on_ButtonRefresh_clicked()
{
  if (ComboTransfers->currentIndex() < 0)
    return;

  const ezString sName = ComboTransfers->currentText().toUtf8().data();

  auto it = m_Transfers.Find(sName);

  if (!it.IsValid())
    return;

  ezTelemetryMessage msg;
  msg.SetMessageID('DTRA', 'REQ');
  msg.GetWriter() << it.Key();
  ezTelemetry::SendToServer(msg);
}

void ezQtDataWidget::on_ComboTransfers_currentIndexChanged(int index)
{
  ComboItems->clear();

  if (index < 0)
    return;

  ezString sName = ComboTransfers->currentText().toUtf8().data();

  auto itTransfer = m_Transfers.Find(sName);

  if (!itTransfer.IsValid())
    return;

  ComboItems->blockSignals(true);

  for (auto itItem = itTransfer.Value().m_Items.GetIterator(); itItem.IsValid(); ++itItem)
  {
    ComboItems->addItem(itItem.Key().GetData());
  }

  ComboItems->setCurrentIndex(0);
  ComboItems->blockSignals(false);

  on_ComboItems_currentIndexChanged(ComboItems->currentIndex());
}

ezQtDataWidget::TransferDataObject* ezQtDataWidget::GetCurrentItem()
{
  auto Transfer = GetCurrentTransfer();

  if (Transfer == nullptr)
    return nullptr;

  ezString sItem = ComboItems->currentText().toUtf8().data();

  auto itItem = Transfer->m_Items.Find(sItem);
  if (!itItem.IsValid())
    return nullptr;

  return &itItem.Value();
}

ezQtDataWidget::TransferData* ezQtDataWidget::GetCurrentTransfer()
{
  ezString sTransfer = ComboTransfers->currentText().toUtf8().data();

  auto itTransfer = m_Transfers.Find(sTransfer);
  if (!itTransfer.IsValid())
    return nullptr;

  return &itTransfer.Value();
}

void ezQtDataWidget::on_ComboItems_currentIndexChanged(int index)
{
  if (index < 0)
    return;

  auto pItem = GetCurrentItem();

  if (!pItem)
    return;

  const ezString sMime = pItem->m_sMimeType;
  auto& Stream = pItem->m_Storage;

  ezMemoryStreamReader Reader(&Stream);

  if (sMime == "image/rgba8")
  {
    ezUInt32 uiWidth, uiHeight;
    Reader >> uiWidth;
    Reader >> uiHeight;

    ezDynamicArray<ezUInt8> Image;
    Image.SetCountUninitialized(uiWidth * uiHeight * 4);

    Reader.ReadBytes(&Image[0], Image.GetCount());

    QImage i(&Image[0], uiWidth, uiHeight, QImage::Format_ARGB32);

    LabelImage->setPixmap(QPixmap::fromImage(i));
  }
  else if (sMime == "text/xml")
  {
    ezHybridArray<ezUInt8, 1024> Temp;
    Temp.SetCountUninitialized(Reader.GetByteCount() + 1);

    Reader.ReadBytes(&Temp[0], Reader.GetByteCount());
    Temp[Reader.GetByteCount()] = '\0';

    LabelImage->setText((const char*)&Temp[0]);
  }
  else
  {
    ezStringBuilder sText;
    sText.Format("Unknown Mime-Type '{0}'", sMime);

    LabelImage->setText(sText.GetData());
  }
}

bool ezQtDataWidget::SaveToFile(TransferDataObject& item, const char* szFile)
{
  auto& Stream = item.m_Storage;
  ezMemoryStreamReader Reader(&Stream);

  QFile FileOut(szFile);
  if (!FileOut.open(QIODevice::WriteOnly))
  {
    QMessageBox::warning(this, QLatin1String("Error writing to file"), QLatin1String("Could not open the specified file for writing."),
                         QMessageBox::Ok, QMessageBox::Ok);
    return false;
  }

  ezHybridArray<ezUInt8, 1024> Temp;
  Temp.SetCountUninitialized(Reader.GetByteCount());

  Reader.ReadBytes(&Temp[0], Reader.GetByteCount());

  if (!Temp.IsEmpty())
    FileOut.write((const char*)&Temp[0], Temp.GetCount());

  FileOut.close();
  return true;
}

void ezQtDataWidget::on_ButtonSave_clicked()
{
  auto pItem = GetCurrentItem();

  if (!pItem)
  {
    QMessageBox::information(this, QLatin1String("ezInspector"), QLatin1String("No valid item selected."), QMessageBox::Ok,
                             QMessageBox::Ok);
    return;
  }

  QString sFilter;

  if (!pItem->m_sExtension.IsEmpty())
  {
    sFilter = "Default (*.";
    sFilter.append(pItem->m_sExtension.GetData());
    sFilter.append(");;");
  }

  sFilter.append("All Files (*.*)");

  QString sResult = QFileDialog::getSaveFileName(this, QLatin1String("Save Data"), pItem->m_sFileName.GetData(), sFilter);

  if (sResult.isEmpty())
    return;

  pItem->m_sFileName = sResult.toUtf8().data();

  SaveToFile(*pItem, pItem->m_sFileName.GetData());
}

void ezQtDataWidget::on_ButtonOpen_clicked()
{
  auto pItem = GetCurrentItem();

  if (!pItem)
  {
    QMessageBox::information(this, QLatin1String("ezInspector"), QLatin1String("No valid item selected."), QMessageBox::Ok,
                             QMessageBox::Ok);
    return;
  }

  if (pItem->m_sFileName.IsEmpty())
    on_ButtonSave_clicked();

  if (pItem->m_sFileName.IsEmpty())
    return;

  SaveToFile(*pItem, pItem->m_sFileName.GetData());

  if (!QDesktopServices::openUrl(QUrl(pItem->m_sFileName.GetData())))
    QMessageBox::information(
        this, QLatin1String("ezInspector"),
        QLatin1String("Could not open the file. There is probably no application registered to handle this file type."), QMessageBox::Ok,
        QMessageBox::Ok);
}

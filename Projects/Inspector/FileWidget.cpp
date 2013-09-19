#include <Inspector/FileWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <MainWindow.moc.h>
#include <Foundation/IO/OSFile.h>
#include <qgraphicsitem.h>

ezFileWidget* ezFileWidget::s_pWidget = NULL;

ezFileWidget::ezFileWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

  ResetStats();
}

void ezFileWidget::ResetStats()
{
  m_bUpdateTable = true;
  m_FileOps.Clear();
  m_FileOps.Reserve(10000);

  Table->clear();

  {
    Table->setColumnCount(5);

    QStringList Headers;
    Headers.append(" # ");
    Headers.append(" Operation ");
    Headers.append(" Duration (sec)");
    Headers.append(" Bytes ");
    Headers.append(" File ");

    Table->setHorizontalHeaderLabels(Headers);
    Table->horizontalHeader()->show();
  }

  Table->resizeColumnsToContents();
  Table->sortByColumn(0, Qt::DescendingOrder);
}

void ezFileWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage('FILE', Msg) == EZ_SUCCESS)
  {
    s_pWidget->m_bUpdateTable = true;

    switch (Msg.GetMessageID())
    {
    case 'OPEN':
      {
        ezInt32 iFileID = 0;
        ezUInt8 uiMode = 0;
        bool bSuccess = false;
        double dTime = 0.0f;

        Msg.GetReader() >> iFileID;

        FileOpData& data = s_pWidget->m_FileOps[iFileID];
        //data.m_StartTime = ezSystemTime::Now();

        Msg.GetReader() >> data.m_sFile;
        Msg.GetReader() >> uiMode;
        Msg.GetReader() >> bSuccess;

        switch (uiMode)
        {
        case ezFileMode::Write:
        case ezFileMode::Append:
          data.m_State = bSuccess ? FileOpState::OpenWriting : FileOpState::OpenWritingFailed;
          break;
        case ezFileMode::Read:
          data.m_State = bSuccess ? FileOpState::OpenReading: FileOpState::OpenReadingFailed;
          break;
        }

        Msg.GetReader() >> dTime;
        data.m_BlockedDuration += ezTime::Seconds(dTime);
      }
      break;
    case 'CLOS':
      {
        ezInt32 iFileID;
        double dTime;

        Msg.GetReader() >> iFileID;

        FileOpData& data = s_pWidget->m_FileOps[iFileID];

        Msg.GetReader() >> dTime;
        data.m_BlockedDuration += ezTime::Seconds(dTime);

        switch (data.m_State)
        {
        case FileOpState::OpenReading:
          data.m_State = FileOpState::ClosedReading;
          break;
        case FileOpState::OpenWriting:
          data.m_State = FileOpState::ClosedWriting;
          break;
        }
      }
      break;
    case 'WRIT':
      {
        ezInt32 iFileID = 0;
        ezUInt64 uiSize;
        bool bSuccess = false;
        double dTime = 0;

        Msg.GetReader() >> iFileID;

        FileOpData& data = s_pWidget->m_FileOps[iFileID];

        Msg.GetReader() >> uiSize;
        Msg.GetReader() >> bSuccess;
        Msg.GetReader() >> dTime;

        data.m_BlockedDuration += ezTime::Seconds(dTime);
        data.m_uiBytesAccessed += uiSize;

        if (!bSuccess)
          data.m_State = FileOpState::OpenWritingFailed;
      }
      break;
    case 'READ':
      {
        ezInt32 iFileID = 0;
        ezUInt64 uiSize;
        ezUInt64 uiRead;
        double dTime = 0;

        Msg.GetReader() >> iFileID;

        FileOpData& data = s_pWidget->m_FileOps[iFileID];

        Msg.GetReader() >> uiSize;
        Msg.GetReader() >> uiRead;
        Msg.GetReader() >> dTime;

        data.m_BlockedDuration += ezTime::Seconds(dTime);
        data.m_uiBytesAccessed += uiRead;
      }
      break;

    case 'EXST':
      {
        ezInt32 iFileID = 0;
        double dTime = 0;
        bool bSuccess;

        Msg.GetReader() >> iFileID;

        FileOpData& data = s_pWidget->m_FileOps[iFileID];
        //data.m_StartTime = ezSystemTime::Now();

        Msg.GetReader() >> data.m_sFile;
        Msg.GetReader() >> bSuccess;
        Msg.GetReader() >> dTime;

        data.m_BlockedDuration += ezTime::Seconds(dTime);
        
        data.m_State = bSuccess ? FileOpState::FileExists : FileOpState::FileExistsFailed;
      }
      break;

    case 'DEL':
      {
        ezInt32 iFileID = 0;
        double dTime = 0;
        bool bSuccess;

        Msg.GetReader() >> iFileID;

        FileOpData& data = s_pWidget->m_FileOps[iFileID];
        //data.m_StartTime = ezSystemTime::Now();

        Msg.GetReader() >> data.m_sFile;
        Msg.GetReader() >> bSuccess;
        Msg.GetReader() >> dTime;

        data.m_BlockedDuration += ezTime::Seconds(dTime);
        
        data.m_State = bSuccess ? FileOpState::FileDelete: FileOpState::FileDeleteFailed;
      }
      break;

    case 'CDIR':
      {
        ezInt32 iFileID = 0;
        double dTime = 0;
        bool bSuccess;

        Msg.GetReader() >> iFileID;

        FileOpData& data = s_pWidget->m_FileOps[iFileID];
        //data.m_StartTime = ezSystemTime::Now();

        Msg.GetReader() >> data.m_sFile;
        Msg.GetReader() >> bSuccess;
        Msg.GetReader() >> dTime;

        data.m_BlockedDuration += ezTime::Seconds(dTime);
        
        data.m_State = bSuccess ? FileOpState::CreateDirs: FileOpState::CreateDirsFailed;
      }
      break;

    case 'COPY':
      {
        ezInt32 iFileID = 0;
        double dTime = 0;
        bool bSuccess;

        ezString sFile1, sFile2;

        Msg.GetReader() >> iFileID;

        FileOpData& data = s_pWidget->m_FileOps[iFileID];
        //data.m_StartTime = ezSystemTime::Now();

        Msg.GetReader() >> sFile1;
        Msg.GetReader() >> sFile2;
        Msg.GetReader() >> bSuccess;
        Msg.GetReader() >> dTime;

        ezStringBuilder s;
        s.Format("'%s' -> '%s'", sFile1.GetData(), sFile2.GetData());
        data.m_sFile = s.GetData();

        data.m_BlockedDuration += ezTime::Seconds(dTime);
        
        data.m_State = bSuccess ? FileOpState::FileCopy: FileOpState::FileCopyFailed;
      }
      break;
    }
  }
}

const char* ezFileWidget::GetStateString(FileOpState State) const
{
  switch (State)
  {
  case FileOpState::ClosedReading:      return "Read";
  case FileOpState::ClosedWriting:      return "Write";
  case FileOpState::CreateDirs:         return "MakeDir";
  case FileOpState::CreateDirsFailed:   return "MakeDir (fail)";
  case FileOpState::FileCopy:           return "Copy";
  case FileOpState::FileCopyFailed:     return "Copy (Failed)";
  case FileOpState::FileDelete:         return "Delete";
  case FileOpState::FileDeleteFailed:   return "Delete (Failed)";
  case FileOpState::FileExists:         return "Exists";
  case FileOpState::FileExistsFailed:   return "Exists (not)";
  case FileOpState::OpenReading:        return "Read (Open)";
  case FileOpState::OpenReadingFailed:  return "Read (Failed)";
  case FileOpState::OpenWriting:        return "Write (Open)";
  case FileOpState::OpenWritingFailed:  return "Write (Failed)";
  }

  return "";
}

void ezFileWidget::UpdateTable()
{
  if (!m_bUpdateTable)
    return;

  m_bUpdateTable = false;

  Table->blockSignals(true);
  Table->setSortingEnabled(false);
  Table->clear();

  {
    Table->setColumnCount(5);

    QStringList Headers;
    Headers.append(" # ");
    Headers.append(" Operation ");
    Headers.append(" Duration (sec) ");
    Headers.append(" Bytes ");
    Headers.append(" File ");

    Table->setHorizontalHeaderLabels(Headers);
    Table->horizontalHeader()->show();
  }

  const ezUInt32 uiMaxRows = m_FileOps.GetCount();
  Table->setRowCount(uiMaxRows);

  ezUInt32 uiRow = 0;
  for (ezHashTable<ezUInt32, FileOpData>::Iterator it = m_FileOps.GetIterator(); it.IsValid(); ++it, ++uiRow)
  {
    QTableWidgetItem* pItem;

    pItem = new QTableWidgetItem();
    pItem->setData(Qt::DisplayRole, QVariant(it.Key()));
    Table->setItem(uiRow, 0, pItem);

    pItem = new QTableWidgetItem();
    pItem->setData(Qt::DisplayRole, QVariant(GetStateString(it.Value().m_State)));
    Table->setItem(uiRow, 1, pItem);

    pItem = new QTableWidgetItem();
    pItem->setData(Qt::DisplayRole, QVariant(it.Value().m_BlockedDuration.GetSeconds()));
    Table->setItem(uiRow, 2, pItem);

    pItem = new QTableWidgetItem();
    pItem->setData(Qt::DisplayRole, QVariant(it.Value().m_uiBytesAccessed));
    Table->setItem(uiRow, 3, pItem);

    pItem = new QTableWidgetItem();
    pItem->setData(Qt::DisplayRole, QVariant(it.Value().m_sFile.GetData()));
    Table->setItem(uiRow, 4, pItem);
  }

  Table->setSortingEnabled(true);
  Table->blockSignals(false);
}

void ezFileWidget::UpdateStats()
{
  if (!m_bUpdateTable)
    return;

  UpdateTable();
}


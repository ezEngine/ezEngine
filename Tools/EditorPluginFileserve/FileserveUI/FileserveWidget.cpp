#include <PCH.h>
#include <EditorPluginFileserve/FileserveUI/FileserveWidget.moc.h>
#include <EditorPluginFileserve/FileserveUI/ActivityModel.moc.h>
#include <EditorPluginFileserve/FileserveUI/AllFilesModel.moc.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <QMessageBox>
#include <QTimer>

ezQtFileserveWidget::ezQtFileserveWidget(QWidget *parent /*= nullptr*/)
{
  setupUi(this);
  Progress->reset();
  m_pActivityModel = new ezQtFileserveActivityModel(this);
  m_pAllFilesModel = new ezQtFileserveAllFilesModel(this);

  ActivityList->setModel(m_pActivityModel);
  AllFilesList->setModel(m_pAllFilesModel);

  ActivityList->horizontalHeader()->setVisible(true);
  AllFilesList->horizontalHeader()->setVisible(true);

  {
    QHeaderView *verticalHeader = ActivityList->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader->setDefaultSectionSize(24);
  }

  {
    QHeaderView *verticalHeader = AllFilesList->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader->setDefaultSectionSize(24);
  }


  connect(m_pActivityModel, SIGNAL(rowsInserted(QModelIndex, int, int)), ActivityList, SLOT(scrollToBottom()));

  if (ezFileserver::GetSingleton())
  {
    ezFileserver::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtFileserveWidget::FileserverEventHandler, this));
  }
  else
  {
    setEnabled(false);
  }
}

ezQtFileserveWidget::~ezQtFileserveWidget()
{
  if (ezFileserver::GetSingleton())
  {
    ezFileserver::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtFileserveWidget::FileserverEventHandler, this));
  }
}

void ezQtFileserveWidget::on_StartServerButton_clicked()
{
  if (ezFileserver::GetSingleton())
  {
    if (m_bServerRunning)
    {
      if (QMessageBox::question(this, "Stop Server?", "Stop Server?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
        return;

      ezFileserver::GetSingleton()->StopServer();
      m_bServerRunning = false;
      StartServerButton->setText("Start Server");
    }
    else
    {
      ezFileserver::GetSingleton()->StartServer();
      m_bServerRunning = true;
      StartServerButton->setText("Stop Server");
    }
  }
}

void ezQtFileserveWidget::on_ClearActivityButton_clicked()
{
  m_pActivityModel->Clear();
}

void ezQtFileserveWidget::on_ClearAllFilesButton_clicked()
{
  m_pAllFilesModel->Clear();
}

void ezQtFileserveWidget::FileserverEventHandler(const ezFileserverEvent& e)
{
  switch (e.m_Type)
  {
  case ezFileserverEvent::Type::ServerStarted:
    LogActivity("", ezFileserveActivityType::StartServer);
    break;

  case ezFileserverEvent::Type::ServerStopped:
    LogActivity("", ezFileserveActivityType::StopServer);
    break;

  case ezFileserverEvent::Type::ConnectedNewClient:
    LogActivity("", ezFileserveActivityType::ClientConnect);
    break;

  case ezFileserverEvent::Type::MountDataDir:
    LogActivity(e.m_szPath, ezFileserveActivityType::Mount);
    break;

  case ezFileserverEvent::Type::UnmountDataDir:
    LogActivity(e.m_szDataDirRootName, ezFileserveActivityType::Unmount);
    break;

  case ezFileserverEvent::Type::FileDownloadRequest:
    {
      m_pAllFilesModel->AddAccessedFile(e.m_szPath);
      LogActivity(e.m_szPath, ezFileserveActivityType::ReadFile);
      TransferLabel->setText(QString("Downloading: %1").arg(e.m_szPath));
      m_LastProgressUpdate = ezTime::Now();
    }
    break;

  case ezFileserverEvent::Type::FileDownloading:
    {
      if (ezTime::Now() - m_LastProgressUpdate > ezTime::Milliseconds(100))
      {
        m_LastProgressUpdate = ezTime::Now();
        Progress->setValue((int)(100.0 * e.m_uiSentTotal / e.m_uiSizeTotal));
      }
    }
    break;

  case ezFileserverEvent::Type::FileDownloadFinished:
    {
      TransferLabel->setText(QString());
      Progress->reset();
    }
    break;

  case ezFileserverEvent::Type::FileDeleteRequest:
    LogActivity(e.m_szPath, ezFileserveActivityType::DeleteFile);
    break;

  case ezFileserverEvent::Type::FileUploadRequest:
    {
      LogActivity(e.m_szPath, ezFileserveActivityType::WriteFile);
      TransferLabel->setText(QString("Uploading: %1").arg(e.m_szPath));
      m_LastProgressUpdate = ezTime::Now();
    }
    break;

  case ezFileserverEvent::Type::FileUploading:
    {
      if (ezTime::Now() - m_LastProgressUpdate > ezTime::Milliseconds(100))
      {
        m_LastProgressUpdate = ezTime::Now();
        Progress->setValue((int)(100.0 * e.m_uiSentTotal / e.m_uiSizeTotal));
      }
    }
    break;

  case ezFileserverEvent::Type::FileUploadFinished:
    {
      TransferLabel->setText(QString());
      Progress->reset();
    }
    break;
  }
}

void ezQtFileserveWidget::LogActivity(const ezFormatString& text, ezFileserveActivityType type)
{
  auto& item = m_pActivityModel->AppendItem();

  ezStringBuilder tmp;
  item.m_Text = text.GetText(tmp);
  item.m_Type = type;
}


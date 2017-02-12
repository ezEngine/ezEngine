#include <PCH.h>
#include <EditorPluginFileserve/FileserveUI/FileserveWidget.moc.h>
#include <FileservePlugin/Fileserver/Fileserver.h>

ezQtFileserveWidget::ezQtFileserveWidget(QWidget *parent /*= nullptr*/)
{
  setupUi(this);
  Progress->reset();

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

void ezQtFileserveWidget::FileserverEventHandler(const ezFileserverEvent& e)
{
  switch (e.m_Type)
  {
  case ezFileserverEvent::Type::ServerStarted:
    LogActivity("Server started");
    break;

  case ezFileserverEvent::Type::ServerStopped:
    LogActivity("Server stopped");
    break;

  case ezFileserverEvent::Type::ConnectedNewClient:
    LogActivity("Client connected");
    break;

  case ezFileserverEvent::Type::MountDataDir:
    LogActivity(ezFmt("Data directory mounted: {0} = '{1}'", e.m_szDataDirRootName, e.m_szPath));
    break;

  case ezFileserverEvent::Type::UnmountDataDir:
    LogActivity(ezFmt("Data directory unmounted: {0}", e.m_szDataDirRootName));
    break;

  case ezFileserverEvent::Type::FileRequest:
    //LogActivity(ezFmt("File Request: '{0}'", e.m_szPath));
    TransferLabel->setText(QString("Downloading: %1").arg(e.m_szPath));
    break;

  case ezFileserverEvent::Type::FileTranser:
    Progress->setValue((int)(100.0 * e.m_uiSentTotal / e.m_uiSizeTotal));
    break;

  case ezFileserverEvent::Type::FileTranserFinished:
    TransferLabel->setText(QString());
    Progress->reset();
    break;

  case ezFileserverEvent::Type::FileDeleteRequest:
    LogActivity(ezFmt("Delete File Request: '{0}'", e.m_szPath));
    break;

  case ezFileserverEvent::Type::FileUploading:
    if (e.m_uiSizeTotal > 1024 * 10)
    {
      TransferLabel->setText(QString("Uploading: %1").arg(e.m_szPath));
      Progress->setValue((int)(100.0 * e.m_uiSentTotal / e.m_uiSizeTotal));
    }
    break;

  case ezFileserverEvent::Type::FileUploadFinished:
    TransferLabel->setText(QString());
    Progress->reset();
    break;
  }
}

void ezQtFileserveWidget::LogActivity(const ezFormatString& text)
{
  ezStringBuilder tmp;
  QListWidgetItem* pItem = new QListWidgetItem(text.GetText(tmp));
  ActivityList->addItem(pItem);
}


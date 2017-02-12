#include <PCH.h>
#include <EditorPluginFileserve/FileserveUI/FileserveWidget.moc.h>
#include <EditorPluginFileserve/FileserveUI/ActivityModel.moc.h>
#include <EditorPluginFileserve/FileserveUI/AllFilesModel.moc.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <QMessageBox>
#include <QTimer>
#include <QHostAddress>
#include <QAbstractSocket>
#include <QNetworkInterface>
#include <GuiFoundation/Basics.h>

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
    QStringList header;
    header.append("");
    header.append("");
    ClientsList->setHeaderLabels(header);
    ClientsList->setHeaderHidden(false);
  }

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
    const ezUInt16 uiPort = ezFileserver::GetSingleton()->GetPort();

    PortLineEdit->setText(QString::number(uiPort));
  }
  else
  {
    setEnabled(false);
  }

  {
    ezStringBuilder sDisplayString, sFirstIP;
    FindOwnIP(sDisplayString, sFirstIP);

    IpLabel->setText(sDisplayString.GetData());
  }
}

void ezQtFileserveWidget::FindOwnIP(ezStringBuilder& out_Display, ezStringBuilder& out_FirstIP)
{
  ezStringBuilder hardwarename;
  out_Display.Clear();
  out_FirstIP.Clear();

  for (const QNetworkInterface& neti : QNetworkInterface::allInterfaces())
  {
    hardwarename = neti.humanReadableName().toUtf8().data();

    if (!neti.isValid())
      continue;
    if (neti.flags().testFlag(QNetworkInterface::IsLoopBack))
      continue;

    if (!neti.flags().testFlag(QNetworkInterface::IsUp))
      continue;
    if (!neti.flags().testFlag(QNetworkInterface::IsRunning))
      continue;
    if (!neti.flags().testFlag(QNetworkInterface::CanBroadcast))
      continue;

    for (const QNetworkAddressEntry& entry : neti.addressEntries())
    {
      if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol)
        continue;
      if (entry.ip().isLoopback())
        continue;
      if (entry.ip().isMulticast())
        continue;
      if (entry.ip().isNull())
        continue;

      // if we DO find multiple adapters, display them all
      if (!out_Display.IsEmpty())
        out_Display.Append(" | ");

      out_Display.AppendFormat("Adapter: '{0}' = {1}", hardwarename, entry.ip().toString().toUtf8().data());

      if (out_FirstIP.IsEmpty())
        out_FirstIP = entry.ip().toString().toUtf8().data();
    }
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
    if (ezFileserver::GetSingleton()->IsServerRunning())
    {
      if (QMessageBox::question(this, "Stop Server?", "Stop Server?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
        return;

      ezFileserver::GetSingleton()->StopServer();
    }
    else
    {
      QString sPort = PortLineEdit->text();
      bool bOk = false;
      ezUInt32 uiPort = sPort.toUInt(&bOk);

      if (bOk && uiPort <= 0xFFFF)
      {
        ezFileserver::GetSingleton()->SetPort((ezUInt16)uiPort);
        ezFileserver::GetSingleton()->StartServer();
      }
      else
        QMessageBox::information(this, "Invalid Port", "The port must be a number between 0 and 65535", QMessageBox::Ok, QMessageBox::Ok);
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
    {
      LogActivity("", ezFileserveActivityType::StartServer);
      PortLineEdit->setEnabled(false);
      StartServerButton->setText("Stop Server");

      ezStringBuilder sDisplayString, sFirstIP;
      FindOwnIP(sDisplayString, sFirstIP);

      emit ServerStarted(sFirstIP.GetData(), ezFileserver::GetSingleton()->GetPort());
    }
    break;

  case ezFileserverEvent::Type::ServerStopped:
    {
      LogActivity("", ezFileserveActivityType::StopServer);
      PortLineEdit->setEnabled(true);
      StartServerButton->setText("Start Server");

      emit ServerStopped();
    }
    break;

  case ezFileserverEvent::Type::ClientConnected:
    {
      LogActivity("", ezFileserveActivityType::ClientConnect);
      m_Clients[e.m_uiClientID].m_bConnected = true;

      UpdateClientList();
    }
    break;

  case ezFileserverEvent::Type::ClientDisconnected:
    {
      LogActivity("", ezFileserveActivityType::ClientDisconnect);
      m_Clients[e.m_uiClientID].m_bConnected = false;

      UpdateClientList();
    }
    break;

  case ezFileserverEvent::Type::MountDataDir:
    {
      LogActivity(e.m_szPath, ezFileserveActivityType::Mount);

      DataDirInfo& dd = m_Clients[e.m_uiClientID].m_DataDirs.ExpandAndGetRef();
      dd.m_sName = e.m_szName;
      dd.m_sPath = e.m_szPath;

      UpdateClientList();
    }
    break;

  case ezFileserverEvent::Type::UnmountDataDir:
    {
      LogActivity(e.m_szName, ezFileserveActivityType::Unmount);

      auto& dds = m_Clients[e.m_uiClientID].m_DataDirs;
      for (ezUInt32 i = 0; i < dds.GetCount(); ++i)
      {
        if (dds[i].m_sName == e.m_szName)
        {
          dds.RemoveAt(i);
          break;
        }
      }

      UpdateClientList();
    }
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

void ezQtFileserveWidget::UpdateClientList()
{
  ezQtScopedBlockSignals bs(ClientsList);

  ClientsList->clear();

  ezStringBuilder sName;

  for (auto it = m_Clients.GetIterator(); it.IsValid(); ++it)
  {
    QTreeWidgetItem* pClient = new QTreeWidgetItem();

    sName.Format("Client: {0}", it.Key());
    pClient->setText(0, sName.GetData());
    pClient->setText(1, it.Value().m_bConnected ? "connected" : "disconnected");

    ClientsList->addTopLevelItem(pClient);

    for (const DataDirInfo& dd : it.Value().m_DataDirs)
    {
      QTreeWidgetItem* pDir = new QTreeWidgetItem(pClient);

      sName = dd.m_sName.GetData();
      if (!sName.IsEmpty())
        sName.Prepend(":");

      pDir->setText(0, sName.GetData());
      pDir->setText(1, dd.m_sPath.GetData());
    }

    pClient->setExpanded(it.Value().m_bConnected);
  }
}


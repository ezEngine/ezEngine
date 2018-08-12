#include <PCH.h>

#include <EditorPluginFileserve/FileserveUI/ActivityModel.moc.h>
#include <EditorPluginFileserve/FileserveUI/AllFilesModel.moc.h>
#include <EditorPluginFileserve/FileserveUI/FileserveWidget.moc.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GuiFoundation/Basics.h>
#include <QAbstractSocket>
#include <QHostAddress>
#include <QInputDialog>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QSettings>
#include <QTableWidget>
#include <QTimer>

ezQtFileserveWidget::ezQtFileserveWidget(QWidget* parent /*= nullptr*/)
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
    ClientsList->setColumnCount(3);

    QStringList header;
    header.append("");
    header.append("");
    header.append("");
    ClientsList->setHeaderLabels(header);
    ClientsList->setHeaderHidden(false);
  }

  {
    QHeaderView* verticalHeader = ActivityList->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader->setDefaultSectionSize(24);
  }

  {
    QHeaderView* verticalHeader = AllFilesList->verticalHeader();
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
    ezStringBuilder sDisplayString;
    FindOwnIP(sDisplayString);

    IpLabel->setText(sDisplayString.GetData());
  }

  ReloadResourcesButton->setEnabled(false);
  SpecialDirAddButton->setVisible(false);
  SpecialDirBrowseButton->setVisible(false);
  SpecialDirRemoveButton->setVisible(false);

  SpecialDirList->setToolTip(
      "Special directories allow to redirect mount requests from the client to a specific folder on the server.\n\n"
      "Some special directories are built in (e.g. 'sdk', 'user' and 'appdir') but you can add custom ones, if your app needs one.\n"
      "To add special directories, run Fileserve with the command line argument '-specialdirs' followed by the name and the path to a "
      "directory.\n\n"
      "For instance:\n"
      "-specialdirs project \"C:\\path\\to\\project\" secondDir \"d:\\another\\path\"");

  ConfigureSpecialDirectories();

  UpdateSpecialDirectoryUI();

  if (ezCommandLineUtils::GetGlobalInstance()->GetBoolOption("-fs_start"))
  {
    QTimer::singleShot(100, this, &ezQtFileserveWidget::on_StartServerButton_clicked);
  }
}

void ezQtFileserveWidget::FindOwnIP(ezStringBuilder& out_Display, ezHybridArray<ezStringBuilder, 4>* out_AllIPs)
{
  ezStringBuilder hardwarename;
  out_Display.Clear();

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
        out_Display.Append("\n");

      out_Display.AppendFormat("Adapter: '{0}' = {1}", hardwarename, entry.ip().toString().toUtf8().data());

      if (out_AllIPs != nullptr)
      {
        out_AllIPs->PushBack(entry.ip().toString().toUtf8().data());
      }
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
      if (QMessageBox::question(this, "Stop Server?", "Stop Server?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ==
          QMessageBox::No)
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


void ezQtFileserveWidget::on_ReloadResourcesButton_clicked()
{
  if (ezFileserver::GetSingleton())
  {
    ezFileserver::GetSingleton()->BroadcastReloadResourcesCommand();
  }
}

void ezQtFileserveWidget::on_ConnectClient_clicked()
{
  QString sIP;

  {
    QSettings Settings;
    Settings.beginGroup(QLatin1String("Fileserve"));
    sIP = Settings.value("ConnectClientIP", "").toString();
    Settings.endGroup();
  }

  bool ok = false;
  sIP = QInputDialog::getText(this, "Connect to Device", "Device IP:", QLineEdit::Normal, sIP, &ok);
  if (!ok)
    return;

  {
    QSettings Settings;
    Settings.beginGroup(QLatin1String("Fileserve"));
    Settings.setValue("ConnectClientIP", sIP);
    Settings.endGroup();
  }

  ezStringBuilder sDisplayString;
  ezHybridArray<ezStringBuilder, 4> AllIPs;
  FindOwnIP(sDisplayString, &AllIPs);

  if (ezFileserver::SendConnectionInfo(sIP.toUtf8().data(), PortLineEdit->text().toInt(), AllIPs).Succeeded())
  {
    LogActivity(ezFmt("Successfully sent server info to client at '{0}'", sIP.toUtf8().data()), ezFileserveActivityType::Other);
  }
  else
  {
    LogActivity(ezFmt("Failed to connect with client at '{0}'", sIP.toUtf8().data()), ezFileserveActivityType::Other);
  }
}

void ezQtFileserveWidget::FileserverEventHandler(const ezFileserverEvent& e)
{
  switch (e.m_Type)
  {
    case ezFileserverEvent::Type::ServerStarted:
    {
      LogActivity("", ezFileserveActivityType::StartServer);
      PortLineEdit->setEnabled(false);
      ReloadResourcesButton->setEnabled(true);
      StartServerButton->setText("Stop Server");

      ezStringBuilder sDisplayString;
      FindOwnIP(sDisplayString);

      emit ServerStarted(sDisplayString.GetData(), ezFileserver::GetSingleton()->GetPort());
    }
    break;

    case ezFileserverEvent::Type::ServerStopped:
    {
      LogActivity("", ezFileserveActivityType::StopServer);
      PortLineEdit->setEnabled(true);
      ReloadResourcesButton->setEnabled(false);
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

    case ezFileserverEvent::Type::ClientReconnected:
    {
      LogActivity("", ezFileserveActivityType::ClientReconnected);
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
      dd.m_sRedirectedPath = e.m_szRedirectedPath;

      UpdateClientList();
    }
    break;

    case ezFileserverEvent::Type::MountDataDirFailed:
    {
      LogActivity(e.m_szPath, ezFileserveActivityType::MountFailed);

      DataDirInfo& dd = m_Clients[e.m_uiClientID].m_DataDirs.ExpandAndGetRef();
      dd.m_sName = e.m_szName;
      dd.m_sPath = e.m_szPath;
      dd.m_sRedirectedPath = "Failed to mount this directory. Special directory name unknown.";

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
      TransferLabel->setText(QString("Downloading: %1").arg(e.m_szPath));
      m_LastProgressUpdate = ezTime::Now();

      if (e.m_FileState == ezFileserveFileState::NonExistant)
        LogActivity(ezFmt("[N/A] {0}", e.m_szPath), ezFileserveActivityType::ReadFile);

      if (e.m_FileState == ezFileserveFileState::SameHash)
        LogActivity(ezFmt("[HASH] {0}", e.m_szPath), ezFileserveActivityType::ReadFile);

      if (e.m_FileState == ezFileserveFileState::SameTimestamp)
        LogActivity(ezFmt("[TIME] {0}", e.m_szPath), ezFileserveActivityType::ReadFile);

      if (e.m_FileState == ezFileserveFileState::NonExistantEither)
        LogActivity(ezFmt("[N/A] {0}", e.m_szPath), ezFileserveActivityType::ReadFile);

      if (e.m_FileState == ezFileserveFileState::Different)
        LogActivity(ezFmt("({1} KB) {0}", e.m_szPath, ezArgF(e.m_uiSizeTotal / 1024.0f, 1)), ezFileserveActivityType::ReadFile);
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
      LogActivity(ezFmt("({1} KB) {0}", e.m_szPath, ezArgF(e.m_uiSizeTotal / 1024.0f, 1)), ezFileserveActivityType::WriteFile);
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

    case ezFileserverEvent::Type::AreYouThereRequest:
    {
      LogActivity("Client searching for Server", ezFileserveActivityType::Other);
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

void ezQtFileserveWidget::UpdateSpecialDirectoryUI()
{
  QTableWidget* pTable = SpecialDirList;
  ezQtScopedBlockSignals bs(SpecialDirList);

  QStringList header;
  header.append("Special Directory");
  header.append("Path");

  SpecialDirList->clear();
  pTable->setColumnCount(2);
  pTable->setRowCount(m_SpecialDirectories.GetCount() + 3);
  pTable->horizontalHeader()->setStretchLastSection(true);
  pTable->verticalHeader()->setDefaultSectionSize(24);
  pTable->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  pTable->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  pTable->verticalHeader()->setHidden(true);
  pTable->setHorizontalHeaderLabels(header);

  ezStringBuilder sResolved;
  QTableWidgetItem* pItem;

  ezUInt32 row = 0;

  for (ezUInt32 i = 0; i < m_SpecialDirectories.GetCount(); ++i, ++row)
  {
    pItem = new QTableWidgetItem();
    pItem->setText(m_SpecialDirectories[i].m_sName.GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pTable->setItem(row, 0, pItem);

    pItem = new QTableWidgetItem();
    pItem->setText(m_SpecialDirectories[i].m_sPath.GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pTable->setItem(row, 1, pItem);
  }

  {
    ezFileSystem::ResolveSpecialDirectory(">sdk", sResolved);

    pItem = new QTableWidgetItem();
    pItem->setText("sdk");
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pTable->setItem(row, 0, pItem);

    pItem = new QTableWidgetItem();
    pItem->setText(sResolved.GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pTable->setItem(row, 1, pItem);

    ++row;
  }

  {
    ezFileSystem::ResolveSpecialDirectory(">user", sResolved);

    pItem = new QTableWidgetItem();
    pItem->setText("user");
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pTable->setItem(row, 0, pItem);

    pItem = new QTableWidgetItem();
    pItem->setText(sResolved.GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pTable->setItem(row, 1, pItem);

    ++row;
  }

  {
    ezFileSystem::ResolveSpecialDirectory(">appdir", sResolved);

    pItem = new QTableWidgetItem();
    pItem->setText("appdir");
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pTable->setItem(row, 0, pItem);

    pItem = new QTableWidgetItem();
    pItem->setText(sResolved.GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pTable->setItem(row, 1, pItem);

    ++row;
  }
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
      pDir->setText(2, dd.m_sRedirectedPath.GetData());

      if (dd.m_sRedirectedPath.StartsWith("Failed"))
        pDir->setTextColor(2, QColor::fromRgb(255, 0, 0));
    }

    pClient->setExpanded(it.Value().m_bConnected);
  }

  ClientsList->resizeColumnToContents(2);
  ClientsList->resizeColumnToContents(1);
  ClientsList->resizeColumnToContents(0);
}

void ezQtFileserveWidget::ConfigureSpecialDirectories()
{
  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();
  const ezUInt32 uiArgs = pCmd->GetStringOptionArguments("-specialdirs");

  ezStringBuilder sDir, sPath;

  for (ezUInt32 i = 0; i < uiArgs; i += 2)
  {
    sDir = pCmd->GetStringOption("-specialdirs", i, "");
    sPath = pCmd->GetStringOption("-specialdirs", i + 1, "");

    if (sDir.IsEmpty() || sPath.IsEmpty())
      continue;

    ezFileSystem::SetSpecialDirectory(sDir, sPath);
    sPath.MakeCleanPath();

    auto& sd = m_SpecialDirectories.ExpandAndGetRef();
    sd.m_sName = sDir;
    sd.m_sPath = sPath;
  }
}

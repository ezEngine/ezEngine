#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/Dialogs/ModifiedDocumentsDlg.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <QMainWindow>
#include <QSettings>
#include <QTimer>
#include <qstylefactory.h>
#include <QFileDialog>
#include <QClipboard>


ezEditorApp* ezEditorApp::s_pInstance = nullptr;

void SetStyleSheet()
{
  QApplication::setStyle(QStyleFactory::create("fusion"));
  QPalette palette;

  palette.setColor(QPalette::WindowText, QColor(200, 200, 200, 255));
  palette.setColor(QPalette::Button, QColor(100, 100, 100, 255));
  palette.setColor(QPalette::Light, QColor(97, 97, 97, 255));
  palette.setColor(QPalette::Midlight, QColor(59, 59, 59, 255));
  palette.setColor(QPalette::Dark, QColor(37, 37, 37, 255));
  palette.setColor(QPalette::Mid, QColor(45, 45, 45, 255));
  palette.setColor(QPalette::Text, QColor(200, 200, 200, 255));
  palette.setColor(QPalette::BrightText, QColor(37, 37, 37, 255));
  palette.setColor(QPalette::ButtonText, QColor(200, 200, 200, 255));
  palette.setColor(QPalette::Base, QColor(42, 42, 42, 255));
  palette.setColor(QPalette::Window, QColor(68, 68, 68, 255));
  palette.setColor(QPalette::Shadow, QColor(0, 0, 0, 255));
  palette.setColor(QPalette::Highlight, QColor(103, 141, 178, 255));
  palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255, 255));
  palette.setColor(QPalette::Link, QColor(0, 0, 238, 255));
  palette.setColor(QPalette::LinkVisited, QColor(82, 24, 139, 255));
  palette.setColor(QPalette::AlternateBase, QColor(46, 46, 46, 255));
  QBrush NoRoleBrush(QColor(0, 0, 0, 255), Qt::NoBrush);
  palette.setBrush(QPalette::NoRole, NoRoleBrush);
  palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 220, 255));
  palette.setColor(QPalette::ToolTipText, QColor(0, 0, 0, 255));

  palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128, 255));
  palette.setColor(QPalette::Disabled, QPalette::Button, QColor(80, 80, 80, 255));
  palette.setColor(QPalette::Disabled, QPalette::Text, QColor(105, 105, 105, 255));
  palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255, 255));
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128, 255));
  palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(86, 117, 148, 255));

  QApplication::setPalette(palette);
}

ezEditorApp::ezEditorApp() :
  s_RecentProjects(5),
  s_RecentDocuments(50)
{
  s_pInstance = this;

  ezUIServices::SetApplicationName("ezEditor");
  s_sUserName = "DefaultUser";
  s_pQtApplication = nullptr;
  s_pEngineViewProcess = nullptr;

  m_pTimer = new QTimer(nullptr);

}

ezEditorApp::~ezEditorApp()
{
  delete m_pTimer;
  m_pTimer = nullptr;
  s_pInstance = nullptr;
}

void ezEditorApp::StartupEditor(const char* szAppName, const char* szUserName, int argc, char** argv)
{
  ezString sApplicationName = ezCommandLineUtils::GetInstance()->GetStringOption("-appname", 0, szAppName);
  ezUIServices::SetApplicationName(sApplicationName);

  s_sUserName = szUserName;

  RegisterPluginNameForSettings("-Main-");

  s_pQtApplication = new QApplication(argc, argv);
  QLocale::setDefault(QLocale(QLocale::English));

  s_pEngineViewProcess = new ezEditorEngineProcessConnection;

  s_pEngineViewProcess->SetWaitForDebugger(ezCommandLineUtils::GetInstance()->GetBoolOption("-debug"));

  QCoreApplication::setOrganizationDomain("www.ezEngine.net");
  QCoreApplication::setOrganizationName("ezEngine Project");
  QCoreApplication::setApplicationName(ezUIServices::GetApplicationName());
  QCoreApplication::setApplicationVersion("1.0.0");

  SetStyleSheet();

  ezContainerWindow* pContainer = new ezContainerWindow();
  pContainer->show();

  ezDocumentManagerBase::s_Requests.AddEventHandler(ezMakeDelegate(&ezEditorApp::DocumentManagerRequestHandler, this));
  ezDocumentManagerBase::s_Events.AddEventHandler(ezMakeDelegate(&ezEditorApp::DocumentManagerEventHandler, this));
  ezDocumentBase::s_EventsAny.AddEventHandler(ezMakeDelegate(&ezEditorApp::DocumentEventHandler, this));
  ezToolsProject::s_Requests.AddEventHandler(ezMakeDelegate(&ezEditorApp::ProjectRequestHandler, this));
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezEditorApp::ProjectEventHandler, this));
  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezMakeDelegate(&ezEditorApp::EngineProcessMsgHandler, this));
  ezDocumentWindow::s_Events.AddEventHandler(ezMakeDelegate(&ezEditorApp::DocumentWindowEventHandler, this));

  ezStartup::StartupCore();

  ezStringBuilder sAppDir = ezOSFile::GetApplicationDirectory();
  sAppDir.AppendPath("../../../Shared/Tools", ezUIServices::GetApplicationName());

  ezOSFile osf;
  osf.CreateDirectoryStructure(sAppDir);

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory("", ezFileSystem::AllowWrites, "App"); // for absolute paths
  ezFileSystem::AddDataDirectory(sAppDir.GetData(), ezFileSystem::AllowWrites, "App"); // for everything relative

  m_LogHTML.BeginLog("Log_Editor.htm", "ezEditor");

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLoggingEvent::Handler(&ezLogWriter::HTML::LogMessageHandler, &m_LogHTML));
 

  ezUIServices::GetInstance()->LoadState();

  LoadRecentFiles();

  CreatePanels();

  LoadPlugins();

  // first open the project, so that the data directory list is read
  if (!s_RecentProjects.GetFileList().IsEmpty())
  {
    CreateOrOpenProject(false, s_RecentProjects.GetFileList()[0]);
  }

  // now open the last document, which might be outside the main project folder, but in an allowed data directory
  if (!s_RecentDocuments.GetFileList().IsEmpty())
  {
    CreateOrOpenDocument(false, s_RecentDocuments.GetFileList()[0]);
  }

  if (ezDocumentWindow::GetAllDocumentWindows().IsEmpty())
  {
    ShowSettingsDocument();
  }
}

void ezEditorApp::ShutdownEditor()
{
  ezToolsProject::CloseProject();

  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezMakeDelegate(&ezEditorApp::EngineProcessMsgHandler, this));
  ezToolsProject::s_Requests.RemoveEventHandler(ezMakeDelegate(&ezEditorApp::ProjectRequestHandler, this));
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezEditorApp::ProjectEventHandler, this));
  ezDocumentBase::s_EventsAny.RemoveEventHandler(ezMakeDelegate(&ezEditorApp::DocumentEventHandler, this));
  ezDocumentManagerBase::s_Requests.RemoveEventHandler(ezMakeDelegate(&ezEditorApp::DocumentManagerRequestHandler, this));
  ezDocumentManagerBase::s_Events.RemoveEventHandler(ezMakeDelegate(&ezEditorApp::DocumentManagerEventHandler, this));
  ezDocumentWindow::s_Events.RemoveEventHandler(ezMakeDelegate(&ezEditorApp::DocumentWindowEventHandler, this));

  SaveSettings();

  ezUIServices::GetInstance()->SaveState();

  CloseSettingsDocument();

  while (!ezContainerWindow::GetAllContainerWindows().IsEmpty())
  {
    delete ezContainerWindow::GetAllContainerWindows()[0];
  }

  // HACK to figure out why the panels are not always properly destroyed together with the ContainerWindows
  // if you run into this, please try to figure this out
  // every ezApplicationPanel actually registers itself with a container window in its constructor
  // there its Qt 'parent' is set to the container window (there is only one)
  // that means, when the application is shut down, all ezApplicationPanel instances should get deleted by their parent
  // ie. the container window
  // however, SOMETIMES this does not happen
  // it seems to be related to whether a panel has been opened/closed (ie. shown/hidden), and maybe also with the restored state
  {
    const auto& Panels = ezApplicationPanel::GetAllApplicationPanels();
    ezUInt32 uiNumPanels = Panels.GetCount();

    EZ_ASSERT_DEBUG(uiNumPanels == 0, "Not all panels have been cleaned up correctly");

    for (ezUInt32 i = 0; i < uiNumPanels; ++i)
    {
      ezApplicationPanel* pPanel = Panels[i];
      QObject* pParent = pPanel->parent();
      delete pPanel;
    }
  }


  QCoreApplication::sendPostedEvents();
  qApp->processEvents();

  delete s_pEngineViewProcess;

  // Unload potental plugin referenced clipboard data to prevent crash on shutdown.
  QApplication::clipboard()->clear();
  UnloadPlugins();

  // make sure no one tries to load any further images in parallel
  QtImageCache::StopRequestProcessing(true);

  delete s_pQtApplication;

  m_LogHTML.EndLog();
}

ezInt32 ezEditorApp::RunEditor()
{
  connect(m_pTimer, SIGNAL(timeout()), this, SLOT(SlotTimedUpdate()), Qt::QueuedConnection);
  m_pTimer->start(1);

  ezInt32 ret = s_pQtApplication->exec();

  ezToolsProject::CloseProject();
  return ret;
}

void ezEditorApp::OpenProject(const char* szProject)
{
  QMetaObject::invokeMethod(this, "SlotQueuedOpenProject", Qt::ConnectionType::QueuedConnection,  Q_ARG(QString, szProject));
}

void ezEditorApp::GuiCreateDocument()
{
  GuiCreateOrOpenDocument(true);
}

void ezEditorApp::GuiOpenDocument()
{
  GuiCreateOrOpenDocument(false);
}

void ezEditorApp::OpenDocument(const char* szDocument)
{
  QMetaObject::invokeMethod(this, "SlotQueuedOpenDocument", Qt::ConnectionType::QueuedConnection,  Q_ARG(QString, szDocument));
}

ezDocumentBase* ezEditorApp::OpenDocumentImmediate(const char* szDocument, bool bRequestWindow)
{
  return CreateOrOpenDocument(false, szDocument, bRequestWindow);
}

void ezEditorApp::GuiCreateProject()
{
  QMetaObject::invokeMethod(this, "SlotQueuedCreateOrOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(bool, true));
}

void ezEditorApp::GuiOpenProject()
{
  QMetaObject::invokeMethod(this, "SlotQueuedCreateOrOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(bool, false));
}

void ezEditorApp::SlotQueuedCreateOrOpenProject(bool bCreate)
{
  GuiCreateOrOpenProject(bCreate);
}

void ezEditorApp::CloseProject()
{
  QMetaObject::invokeMethod(this, "SlotQueuedCloseProject", Qt::ConnectionType::QueuedConnection);
}

void ezEditorApp::SlotQueuedOpenProject(QString sProject)
{
  CreateOrOpenProject(false, sProject.toUtf8().data());
}

void ezEditorApp::SlotQueuedOpenDocument(QString sProject)
{
  CreateOrOpenDocument(false, sProject.toUtf8().data());
}

void ezEditorApp::SlotQueuedCloseProject()
{
  // purge the image loading queue when a project is closed, but keep the existing cache
  QtImageCache::StopRequestProcessing(false);

  ezToolsProject::CloseProject();

  // enable image loading again, the queue is purged now
  QtImageCache::EnableRequestProcessing();
}

void ezEditorApp::SlotTimedUpdate()
{
  if (ezEditorEngineProcessConnection::GetInstance())
    ezEditorEngineProcessConnection::GetInstance()->Update();

  m_pTimer->start(1);
}

void ezEditorApp::DocumentManagerEventHandler(const ezDocumentManagerBase::Event& r)
{
  switch (r.m_Type)
  {
  case ezDocumentManagerBase::Event::Type::DocumentWindowRequested:
    {
      s_RecentDocuments.Insert(r.m_pDocument->GetDocumentPath());
      SaveSettings();
    }
    break;
  case ezDocumentManagerBase::Event::Type::DocumentClosing:
    {
      // Clear all document settings when it is closed
      s_DocumentSettings.Remove(r.m_pDocument->GetDocumentPath());
    }
    break;
  }
}

void ezEditorApp::DocumentEventHandler(const ezDocumentBase::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentBase::Event::Type::SaveDocumentMetaState:
    {
      SaveDocumentSettings(e.m_pDocument);
    }
    break;

  case ezDocumentBase::Event::Type::DocumentSaved:
    {
      ezAssetCurator::GetInstance()->NotifyOfPotentialAsset(e.m_pDocument->GetDocumentPath());
    }
    break;
  }
}

void ezEditorApp::DocumentWindowEventHandler(const ezDocumentWindow::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentWindow::Event::WindowClosed:
    {
      // if all windows are closed, show at least the settings window
      if (ezDocumentWindow::GetAllDocumentWindows().GetCount() == 0)
      {
        ShowSettingsDocument();
      }
    }
    break;
  }
}

void ezEditorApp::DocumentManagerRequestHandler(ezDocumentManagerBase::Request& r)
{
  switch (r.m_Type)
  {
  case ezDocumentManagerBase::Request::Type::DocumentAllowedToOpen:
    {
      // if someone else already said no, don't bother to check further
      if (r.m_RequestStatus.m_Result.Failed())
        return;

      if (!ezToolsProject::IsProjectOpen())
      {
        // if no project is open yet, try to open the corresponding one

        ezString sProjectPath = ezToolsProject::FindProjectForDocument(r.m_sDocumentPath);

        // if no project could be located, just reject the request
        if (sProjectPath.IsEmpty())
        {
          r.m_RequestStatus = ezStatus("No project could be opened");
          return;
        }
        else
        {
          // if a project could be found, try to open it
          ezStatus res = ezToolsProject::OpenProject(sProjectPath);

          // if project opening failed, relay that error message
          if (res.m_Result.Failed())
          {
            r.m_RequestStatus = res;
            return;
          }
        }
      }
      else
      {
        if (!ezToolsProject::GetInstance()->IsDocumentInAllowedRoot(r.m_sDocumentPath))
        {
          r.m_RequestStatus = ezStatus("The document is not part of the currently open project");
          return;
        }
      }
    }
    return;
  }
}

void ezEditorApp::EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
  case ezEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<ezUpdateReflectionTypeMsgToEditor>())
      {
        const ezUpdateReflectionTypeMsgToEditor* pMsg = static_cast<const ezUpdateReflectionTypeMsgToEditor*>(e.m_pMsg);
        ezPhantomRttiManager::RegisterType(pMsg->m_desc);
      }
      else if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<ezProjectReadyMsgToEditor>())
      {
        // This message is waited upon (blocking) but does not contain any data.
      }
    }
    break;

  default:
    return;
  }
}

void ezEditorApp::ProjectEventHandler(const ezToolsProject::Event& r)
{
  switch (r.m_Type)
  {
  case ezToolsProject::Event::Type::ProjectOpened:
    {
      SetupDataDirectories();

      ezEditorEngineProcessConnection::GetInstance()->SetFileSystemConfig(m_FileSystemConfig);

      if (ezEditorEngineProcessConnection::GetInstance()->RestartProcess().Failed())
      {
        ezLog::Error("Failed to start the engine process. Project loading incomplete.");
      }

      m_AssetCurator.Initialize(m_FileSystemConfig);

      m_sLastDocumentFolder = ezToolsProject::GetInstance()->GetProjectPath();
      m_sLastProjectFolder = ezToolsProject::GetInstance()->GetProjectPath();
    }
    // fall through

  case ezToolsProject::Event::Type::ProjectClosing:
    {
      // TODO: Write asset cache to file.

      s_RecentProjects.Insert(ezToolsProject::GetInstance()->GetProjectPath());
      SaveSettings();
    }
    break;
  case ezToolsProject::Event::Type::ProjectClosed:
    {
      ezEditorEngineProcessConnection::GetInstance()->ShutdownProcess();

      m_AssetCurator.Deinitialize();

      // make sure to clear all project settings when a project is closed
      s_ProjectSettings.Clear();

      ezApplicationConfig::SetProjectDirectory("");

      s_ReloadProjectRequiredReasons.Clear();
      UpdateGlobalStatusBarMessage();
    }
    break;
  }
}

void ezEditorApp::ProjectRequestHandler(ezToolsProject::Request& r)
{
  switch (r.m_Type)
  {
    case ezToolsProject::Request::Type::CanProjectClose:
    {
      if (r.m_bProjectCanClose == false)
        return;

      ezHybridArray<ezDocumentBase*, 32> ModifiedDocs;

      for (ezDocumentManagerBase* pMan : ezDocumentManagerBase::GetAllDocumentManagers())
      {
        for (ezDocumentBase* pDoc : pMan->GetAllDocuments())
        {
          if (pDoc->IsModified())
            ModifiedDocs.PushBack(pDoc);
        }
      }

      if (!ModifiedDocs.IsEmpty())
      {
        ezModifiedDocumentsDlg dlg(QApplication::activeWindow(), ModifiedDocs);
        if (dlg.exec() == 0)
          r.m_bProjectCanClose = false;
      }
    }
      return;
  }
}

void ezRecentFilesList::Insert(const char* szFile)
{
  ezStringBuilder sCleanPath = szFile;
  sCleanPath.MakeCleanPath();

  ezString s = sCleanPath;

  m_Files.Remove(s);
  m_Files.PushFront(s);

  if (m_Files.GetCount() > m_uiMaxElements)
    m_Files.SetCount(m_uiMaxElements);
}

void ezRecentFilesList::Save(const char* szFile)
{
  ezFileWriter File;
  if (File.Open(szFile).Failed())
    return;

  for (const ezString& s : m_Files)
  {
    File.WriteBytes(s.GetData(), s.GetElementCount());
    File.WriteBytes("\n", sizeof(char));
  }
}

void ezRecentFilesList::Load(const char* szFile)
{
  m_Files.Clear();

  ezFileReader File;
  if (File.Open(szFile).Failed())
    return;

  ezStringBuilder sAllLines;
  sAllLines.ReadAll(File);

  ezHybridArray<ezStringView, 16> Lines;
  sAllLines.Split(false, Lines, "\n");

  for (const ezStringView& sv : Lines)
  {
    m_Files.PushBack(sv);
  }
}

ezString ezEditorApp::GetDocumentDataFolder(const char* szDocument)
{
  ezStringBuilder sPath = szDocument;
  sPath.Append("_data");

  return sPath;
}

ezString ezEditorApp::BuildDocumentTypeFileFilter(bool bForCreation)
{
  ezStringBuilder sAllFilters;
  const char* sepsep = "";

  if (!bForCreation)
  {
    sAllFilters = "All Files (*.*)";
    sepsep = ";;";
  }

  for (ezDocumentManagerBase* pMan : ezDocumentManagerBase::GetAllDocumentManagers())
  {
    ezHybridArray<ezDocumentTypeDescriptor, 4> Types;
    pMan->GetSupportedDocumentTypes(Types);

    for (const ezDocumentTypeDescriptor& desc : Types)
    {
      if (bForCreation && !desc.m_bCanCreate)
        continue;

      if (desc.m_sFileExtensions.IsEmpty())
        continue;

      sAllFilters.Append(sepsep, desc.m_sDocumentTypeName, " (");
      sepsep = ";;";

      const char* sep = "";

      for (const ezString& ext : desc.m_sFileExtensions)
      {
        sAllFilters.Append(sep, "*.", ext);
        sep = "; ";
      }

      sAllFilters.Append(")");

      desc.m_sDocumentTypeName;
    }
  }

  return sAllFilters;
}

void ezEditorApp::GuiCreateOrOpenDocument(bool bCreate)
{
  const ezString sAllFilters = BuildDocumentTypeFileFilter(bCreate);

  if (sAllFilters.IsEmpty())
  {
    ezUIServices::MessageBoxInformation("No file types are currently known. Load plugins to add file types.");
    return;
  }

  static QString sSelectedExt;
  const QString sDir = QString::fromUtf8(m_sLastDocumentFolder.GetData());

  ezString sFile;

  if (bCreate)
    sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Create Document"), sDir, QString::fromUtf8(sAllFilters.GetData()), &sSelectedExt).toUtf8().data();
  else
    sFile = QFileDialog::getOpenFileName(QApplication::activeWindow(), QLatin1String("Open Document"), sDir, QString::fromUtf8(sAllFilters.GetData()), &sSelectedExt).toUtf8().data();

  if (sFile.IsEmpty())
    return;

  m_sLastDocumentFolder = ezPathUtils::GetFileDirectory(sFile);

  ezDocumentManagerBase* pManToCreate = nullptr;
  ezDocumentTypeDescriptor DescToCreate;

  if (ezDocumentManagerBase::FindDocumentTypeFromPath(sFile, bCreate, pManToCreate, &DescToCreate).Succeeded())
  {
    sSelectedExt = DescToCreate.m_sDocumentTypeName;
  }

  CreateOrOpenDocument(bCreate, sFile);
}

ezDocumentBase* ezEditorApp::CreateOrOpenDocument(bool bCreate, const char* szFile, bool bRequestWindow)
{
  ezDocumentManagerBase* pManToCreate = nullptr;
  ezDocumentTypeDescriptor DescToCreate;

  if (ezDocumentManagerBase::FindDocumentTypeFromPath(szFile, bCreate, pManToCreate, &DescToCreate).Failed())
  {
    ezStringBuilder sTemp = szFile;
    ezStringBuilder sExt = sTemp.GetFileExtension();

    sTemp.Format("The selected file extension '%s' is not registered with any known type.\nCannot open file '%s'", sExt.GetData(), szFile);

    ezUIServices::MessageBoxWarning(sTemp);
    return nullptr;
  }

  // does the same document already exist and is open ?
  ezDocumentBase* pDocument = pManToCreate->GetDocumentByPath(szFile);

  if (!pDocument)
  {
    ezStatus res;

    if (bCreate)
      res = pManToCreate->CreateDocument(DescToCreate.m_sDocumentTypeName, szFile, pDocument, bRequestWindow);
    else
    {
      res = pManToCreate->CanOpenDocument(szFile);

      if (res.m_Result.Succeeded())
      {
        res = pManToCreate->OpenDocument(DescToCreate.m_sDocumentTypeName, szFile, pDocument, bRequestWindow);
      }
    }

    if (res.m_Result.Failed())
    {
      ezStringBuilder s;
      s.Format("Failed to open document: \n'%s'", szFile);

      ezUIServices::MessageBoxStatus(res, s);
      return nullptr;
    }

    EZ_ASSERT_DEV(pDocument != nullptr, "Creation of document type '%s' succeeded, but returned pointer is NULL", DescToCreate.m_sDocumentTypeName.GetData());
  }
  else
  {
    if (bCreate)
    {
      ezUIServices::MessageBoxInformation("The selected document is already open. You need to close the document before you can re-create it.");
      return nullptr;
    }
  }

  if (bRequestWindow)
    ezContainerWindow::EnsureVisibleAnyContainer(pDocument);

  return pDocument;
}

void ezEditorApp::GuiCreateOrOpenProject(bool bCreate)
{
  const QString sDir = QString::fromUtf8(m_sLastProjectFolder.GetData());
  ezStringBuilder sFile;

  const char* szFilter = "ezProject (ezProject)";

  if (bCreate)
    sFile = QFileDialog::getExistingDirectory(QApplication::activeWindow(), QLatin1String("Choose Folder for New Project"), sDir).toUtf8().data();
  else
    sFile = QFileDialog::getOpenFileName(QApplication::activeWindow(), QLatin1String("Open Project"), sDir, QLatin1String(szFilter)).toUtf8().data();

  if (sFile.IsEmpty())
    return;

  if (bCreate)
    sFile.AppendPath("ezProject");

  m_sLastProjectFolder = ezPathUtils::GetFileDirectory(sFile);

  CreateOrOpenProject(bCreate, sFile);
}

void ezEditorApp::CreateOrOpenProject(bool bCreate, const char* szFile)
{
  if (ezToolsProject::IsProjectOpen() && ezToolsProject::GetInstance()->GetProjectPath() == szFile)
  {
    ezUIServices::MessageBoxInformation("The selected project is already open");
    return;
  }

  if (!ezToolsProject::CanCloseProject())
    return;

  ezStatus res;
  if (bCreate)
    res = ezToolsProject::CreateProject(szFile);
  else
    res = ezToolsProject::OpenProject(szFile);

  if (res.m_Result.Failed())
  {
    ezStringBuilder s;
    s.Format("Failed to open project:\n'%s'", szFile);

    ezUIServices::MessageBoxStatus(res, s);
    return;
  }
}





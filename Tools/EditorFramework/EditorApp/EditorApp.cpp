#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Dialogs/ModifiedDocumentsDlg.moc.h>
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
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <QMainWindow>
#include <QSettings>
#include <QTimer>
#include <qstylefactory.h>

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
  s_pEngineViewProcess = new ezEditorEngineProcessConnection;

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

  LoadPlugins();

  if (!s_RecentDocuments.GetFileList().IsEmpty())
  {
    ezContainerWindow::CreateOrOpenDocument(false, s_RecentDocuments.GetFileList()[0]);
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
  QCoreApplication::sendPostedEvents();
  qApp->processEvents();

  delete s_pEngineViewProcess;

  UnloadPlugins();

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

void ezEditorApp::CloseProject()
{
  QMetaObject::invokeMethod(this, "SlotQueuedCloseProject", Qt::ConnectionType::QueuedConnection);
}

void ezEditorApp::OpenProject(const char* szProject)
{
  QMetaObject::invokeMethod(this, "SlotQueuedOpenProject", Qt::ConnectionType::QueuedConnection,  Q_ARG(QString, szProject));
}

void ezEditorApp::OpenDocument(const char* szDocument)
{
  QMetaObject::invokeMethod(this, "SlotQueuedOpenDocument", Qt::ConnectionType::QueuedConnection,  Q_ARG(QString, szDocument));
}

void ezEditorApp::SlotQueuedOpenProject(QString sProject)
{
  ezContainerWindow::CreateOrOpenProject(false, sProject.toUtf8().data());


}

void ezEditorApp::SlotQueuedOpenDocument(QString sProject)
{
  ezContainerWindow::CreateOrOpenDocument(false, sProject.toUtf8().data());
}

void ezEditorApp::SlotQueuedCloseProject()
{
  ezToolsProject::CloseProject();
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
  case ezDocumentManagerBase::Event::Type::DocumentOpened:
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
        ShowSettingsDocument();
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
        if (!ezToolsProject::GetInstance()->IsDocumentInProject(r.m_sDocumentPath))
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
      static ezReflectedTypeDescriptor s_TypeDesc;

      if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<ezUpdateReflectionTypeMsgToEditor>())
      {
        const ezUpdateReflectionTypeMsgToEditor* pMsg = static_cast<const ezUpdateReflectionTypeMsgToEditor*>(e.m_pMsg);

        s_TypeDesc.m_sTypeName = pMsg->m_sTypeName;
        s_TypeDesc.m_sDefaultInitialization = pMsg->m_sDefaultInitialization;
        s_TypeDesc.m_sParentTypeName = pMsg->m_sParentTypeName;
        s_TypeDesc.m_sPluginName = pMsg->m_sPluginName;
        s_TypeDesc.m_Properties.SetCount(pMsg->m_uiNumProperties);

        if (pMsg->m_uiNumProperties == 0)
        {
          ezReflectedTypeManager::RegisterType(s_TypeDesc);
          s_TypeDesc.m_Properties.Clear();
          s_TypeDesc.m_Properties.Compact();
        }
      }
      else if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<ezProjectReadyMsgToEditor>())
      {
        //const ezProjectReadyMsgToEditor* pMsg = static_cast<const ezProjectReadyMsgToEditor*>(e.m_pMsg);


      }
      else if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<ezUpdateReflectionPropertyMsgToEditor>())
      {
        const ezUpdateReflectionPropertyMsgToEditor* pMsg = static_cast<const ezUpdateReflectionPropertyMsgToEditor*>(e.m_pMsg);

        auto& ref = s_TypeDesc.m_Properties[pMsg->m_uiPropertyIndex];
        ref.m_ConstantValue = pMsg->m_ConstantValue;
        ref.m_Flags = ((PropertyFlags::Enum) pMsg->m_Flags);
        ref.m_sName = pMsg->m_sName;
        ref.m_Type = (ezVariant::Type::Enum) pMsg->m_Type;
        ref.m_sType = pMsg->m_sType;

        if (pMsg->m_uiPropertyIndex + 1 == s_TypeDesc.m_Properties.GetCount())
        {
          ezReflectedTypeManager::RegisterType(s_TypeDesc);
          s_TypeDesc.m_Properties.Clear();
          s_TypeDesc.m_Properties.Compact();
        }
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
      ezStringBuilder sPath = ezToolsProject::GetInstance()->GetProjectPath();
      sPath.PathParentDirectory();

      ezApplicationConfig::SetProjectDirectory(sPath);
      m_FileSystemConfig.Load();

      ezEditorEngineProcessConnection::GetInstance()->RestartProcess();
      ezEditorEngineProcessConnection::GetInstance()->WaitForMessage(ezGetStaticRTTI<ezProjectReadyMsgToEditor>());
    }
    // fall through

  case ezToolsProject::Event::Type::ProjectClosing:
    {
      s_RecentProjects.Insert(ezToolsProject::GetInstance()->GetProjectPath());
      SaveSettings();
    }
    break;
  case ezToolsProject::Event::Type::ProjectClosed:
    {
      ezEditorEngineProcessConnection::GetInstance()->ShutdownProcess();

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






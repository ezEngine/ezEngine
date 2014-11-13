#include <PCH.h>
#include <EditorFramework/EditorFramework.h>
#include <EditorFramework/EditorGUI.moc.h>
#include <EditorFramework/Dialogs/DocumentList.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <QMainWindow>
#include <QSettings>
#include <qstylefactory.h>

ezString ezEditorFramework::s_sApplicationName("ezEditor");
ezString ezEditorFramework::s_sUserName("DefaultUser");
//bool ezEditorFramework::s_bContentModified = false;
ezHybridArray<ezContainerWindow*, 4> ezEditorFramework::s_ContainerWindows;
QApplication* ezEditorFramework::s_pQtApplication = nullptr;

ezSet<ezString> ezEditorFramework::s_RestartRequiredReasons;

QMainWindow* ezEditorFramework::GetMainWindow()
{
  return s_ContainerWindows[0];
}

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

void ezEditorFramework::StartupEditor(const char* szAppName, const char* szUserName, int argc, char** argv)
{
  ezCommandLineUtils::GetInstance()->SetCommandLine(argc, (const char**) argv);

  s_sApplicationName = ezCommandLineUtils::GetInstance()->GetStringOption("-appname", 0, szAppName);
  s_sUserName = szUserName;

  s_pQtApplication = new QApplication(argc, argv);

  QCoreApplication::setOrganizationDomain("www.ezEngine.net");
  QCoreApplication::setOrganizationName("ezEngine Project");
  QCoreApplication::setApplicationName(ezEditorFramework::GetApplicationName().GetData());
  QCoreApplication::setApplicationVersion("1.0.0");

  SetStyleSheet();

  //UpdateEditorWindowTitle();

  // load the settings
  //GetSettings(SettingsCategory::Editor);

  s_ContainerWindows.PushBack(new ezContainerWindow());
  s_ContainerWindows[0]->show();

  s_ContainerWindows[0]->ShowSettingsTab();

  ezDocumentManagerBase::s_Requests.AddEventHandler(ezDelegate<void (ezDocumentManagerBase::Request&)>(&ezEditorFramework::DocumentManagerRequestHandler));
  ezEditorProject::s_Requests.AddEventHandler(ezDelegate<void (ezEditorProject::Request&)>(&ezEditorFramework::ProjectRequestHandler));

  ezStartup::StartupCore();

  ezStringBuilder sAppDir = ezOSFile::GetApplicationDirectory();
  sAppDir.AppendPath("..", ezEditorFramework::GetApplicationName().GetData());

  ezOSFile osf;
  osf.CreateDirectoryStructure(sAppDir.GetData());

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory("", ezFileSystem::AllowWrites, "Editor"); // for absolute paths
  ezFileSystem::AddDataDirectory(sAppDir.GetData(), ezFileSystem::AllowWrites, "Editor"); // for everything relative

  ezEditorGUI::GetInstance()->LoadState();

  ezEditorFramework::LoadPlugins();
}

void ezEditorFramework::ShutdownEditor()
{
  ezEditorProject::CloseProject();

  SaveSettings();

  ezEditorProject::s_Requests.RemoveEventHandler(ezDelegate<void (ezEditorProject::Request&)>(&ezEditorFramework::ProjectRequestHandler));
  ezDocumentManagerBase::s_Requests.RemoveEventHandler(ezDelegate<void (ezDocumentManagerBase::Request&)>(&ezEditorFramework::DocumentManagerRequestHandler));

  ezEditorGUI::GetInstance()->SaveState();

  delete s_pQtApplication;
}

ezInt32 ezEditorFramework::RunEditor()
{
  return s_pQtApplication->exec();
}

void ezEditorFramework::DocumentManagerRequestHandler(ezDocumentManagerBase::Request& r)
{
  switch (r.m_Type)
  {
  case ezDocumentManagerBase::Request::Type::DocumentAllowedToOpen:
    {
      // if someone else already said no, don't bother to check further
      if (r.m_RequestStatus.m_Result.Failed())
        return;

      if (!ezEditorProject::IsProjectOpen())
      {
        // if no project is open yet, try to open the corresponding one

        ezString sProjectPath = ezEditorProject::FindProjectForDocument(r.m_sDocumentPath);

        // if no project could be located, just reject the request
        if (sProjectPath.IsEmpty())
        {
          r.m_RequestStatus = ezStatus("No project could be opened");
          return;
        }
        else
        {
          // if a project could be found, try to open it
          ezStatus res = ezEditorProject::OpenProject(sProjectPath);

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
        if (!ezEditorProject::GetInstance()->IsDocumentInProject(r.m_sDocumentPath))
        {
          r.m_RequestStatus = ezStatus("The document is not part of the currently open project");
          return;
        }
      }
    }
    return;
  }
}

void ezEditorFramework::ProjectRequestHandler(ezEditorProject::Request& r)
{
  switch (r.m_Type)
  {
    case ezEditorProject::Request::Type::CanProjectClose:
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
        DocumentList dlg(s_ContainerWindows[0], ModifiedDocs);
        if (dlg.exec() == 0)
          r.m_bProjectCanClose = false;
      }
    }
    return;
  }
}


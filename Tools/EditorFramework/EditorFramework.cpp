#include <PCH.h>
#include <EditorFramework/EditorFramework.h>
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
//ezString ezEditorFramework::s_sUserName("DefaultUser");
//bool ezEditorFramework::s_bContentModified = false;
ezHybridArray<ezContainerWindow*, 4> ezEditorFramework::s_ContainerWindows;
QApplication* ezEditorFramework::s_pQtApplication = nullptr;

//ezEvent<const ezEditorFramework::EditorEvent&> ezEditorFramework::s_EditorEvents;
//
ezEvent<ezEditorFramework::EditorRequest&> ezEditorFramework::s_EditorRequests;

ezSet<ezString> ezEditorFramework::s_RestartRequiredReasons;

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

void ezEditorFramework::StartupEditor(ezStringView sAppName, ezStringView sUserName, int argc, char** argv)
{
  ezCommandLineUtils::GetInstance()->SetCommandLine(argc, (const char**) argv);

  s_pQtApplication = new QApplication(argc, argv);

  QCoreApplication::setOrganizationDomain("www.ezEngine.net");
  QCoreApplication::setOrganizationName("ezEngine Project");
  QCoreApplication::setApplicationName(ezEditorFramework::GetApplicationName().GetData());
  QCoreApplication::setApplicationVersion("1.0.0");

  SetStyleSheet();

  s_sApplicationName = sAppName;
  //s_sUserName = sUserName;
  //UpdateEditorWindowTitle();

  // load the settings
  //GetSettings(SettingsCategory::Editor);

  s_ContainerWindows.PushBack(new ezContainerWindow());
  s_ContainerWindows[0]->show();


  ezStartup::StartupCore();

  ezStringBuilder sAppDir = ezOSFile::GetApplicationDirectory();
  sAppDir.AppendPath("..", ezEditorFramework::GetApplicationName().GetData());

  ezOSFile osf;
  osf.CreateDirectoryStructure(sAppDir.GetData());

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory("", ezFileSystem::AllowWrites, "Editor"); // for absolute paths
  ezFileSystem::AddDataDirectory(sAppDir.GetData(), ezFileSystem::AllowWrites, "Editor"); // for everything relative

  ezEditorFramework::LoadPlugins();

  s_ContainerWindows[0]->RestoreWindowLayout();
}

void ezEditorFramework::ShutdownEditor()
{
  //SaveSettings();

  //CloseProject();

  delete s_pQtApplication;
}

ezInt32 ezEditorFramework::RunEditor()
{
  return s_pQtApplication->exec();
}

/*
void ezEditorFramework::UpdateEditorWindowTitle()
{
ezStringBuilder sTitle = s_sApplicationName;

//if (!s_sScenePath.IsEmpty())
//{
//  sTitle.Append(" - ", s_sScenePath.GetData());
//}
//else
if (!s_sProjectPath.IsEmpty())
{
sTitle.Append(" - ", s_sProjectPath.GetData());
}

if (s_bContentModified)
{
sTitle.Append("*");
}

//if (s_pMainWindow)
//  s_pMainWindow->setWindowTitle(QString::fromUtf8(sTitle.GetData()));
}
*/

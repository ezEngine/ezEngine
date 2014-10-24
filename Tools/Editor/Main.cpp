#include <PCH.h>
#include <Core/Application/Application.h>
#include <Editor/Windows/EditorMainWnd.moc.h>
#include <EditorFramework/EditorFramework.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <QApplication>
#include <qstylefactory.h>
#include <QSettings>

class ezEditorApp : public ezApplication
{
public:

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

  virtual ApplicationExecution Run() override
  {
    int iArgs = GetArgumentCount();
    char** cArgs = (char**) GetArgumentsArray();

    ezCommandLineUtils::GetInstance()->SetCommandLine(GetArgumentCount(), GetArgumentsArray());
    ezEditorFramework::StartupEditor(ezCommandLineUtils::GetInstance()->GetStringOption("-appname", 0, "ezEditor"), "DefaultUser");

    {
      QApplication app(iArgs, cArgs);
      QCoreApplication::setOrganizationDomain("www.ezEngine.net");
      QCoreApplication::setOrganizationName("ezEngine Project");
      QCoreApplication::setApplicationName(ezEditorFramework::GetApplicationName().GetData());
      QCoreApplication::setApplicationVersion("1.0.0");
      SetStyleSheet();

      ezEditorMainWnd MainWindow;
      ezEditorFramework::SetMainWindow(&MainWindow);

      // messy
      ezStartup::StartupCore();

      ezStringBuilder sAppDir = ezOSFile::GetApplicationDirectory();
      sAppDir.AppendPath("..", ezEditorFramework::GetApplicationName().GetData());

      ezOSFile osf;
      osf.CreateDirectoryStructure(sAppDir.GetData());

      ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
      ezFileSystem::AddDataDirectory("", ezFileSystem::AllowWrites, "Editor"); // for absolute paths
      ezFileSystem::AddDataDirectory(sAppDir.GetData(), ezFileSystem::AllowWrites, "Editor"); // for everything relative

      ezEditorFramework::LoadPlugins();

      // *** RESTORE ***
      ezEditorFramework::RestoreWindowLayout();

      MainWindow.show();

      const ezInt32 iReturnCode = app.exec();

      SetReturnCode(iReturnCode);
      ezEditorFramework::SetMainWindow(nullptr);

      ezEditorFramework::ShutdownEditor();
    }

    return ezApplication::Quit;
  }
};

EZ_APPLICATION_ENTRY_POINT(ezEditorApp);


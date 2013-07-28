#include <PCH.h>
#include <QApplication>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Core/Application/Application.h>
#include <vector>
#include <Core/Input/InputManager.h>
#include <InputWindows/InputDeviceWindows.h>
#include <InputXBox360/InputDeviceXBox.h>
#include <InputTest/MainWindow.h>

using namespace std;

ezStatic<ezLogWriter::HTML> g_HtmlLog;


class ezInputTestApp : public ezApplication
{
public:
  virtual void BeforeEngineInit() EZ_OVERRIDE
  {
  }

  virtual void AfterEngineInit() EZ_OVERRIDE
  {
    ezLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);

    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
    ezFileSystem::AddDataDirectory("");
    g_HtmlLog.GetStatic().BeginLog("D:\\InputTestLog.htm", "Input Test");

    
    ezLog::AddLogWriter(ezLogWriter::HTML::LogMessageHandler, &g_HtmlLog.GetStatic());

    ezInputDeviceWindows::LocalizeButtonDisplayNames();

    //g_InputDeviceWindows.SetShowMouseCursor(false);

    //g_InputDeviceXBox360.SetPhysicalToVirtualControllerMapping(0, 1);
    //g_InputDeviceXBox360.SetPhysicalToVirtualControllerMapping(2, -1);
  }

  virtual void BeforeEngineShutdown() EZ_OVERRIDE
  {
    g_HtmlLog.GetStatic().EndLog();
  }

  virtual void AfterEngineShutdown() EZ_OVERRIDE
  {
  }

  virtual ApplicationExecution Run() EZ_OVERRIDE
  {
    int iArgs = GetArgumentCount();
    char** cArgs = (char**) GetArgumentsArray();

    QApplication app(iArgs, cArgs);
    app.setApplicationName("ezQTSample");
    ezMainWindow mainWindow;
    mainWindow.show();
    SetReturnCode(app.exec());

    return ezApplication::Quit;
  }

};

EZ_APPLICATION_ENTRY_POINT(ezInputTestApp);


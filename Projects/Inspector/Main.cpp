#include <Core/Application/Application.h>
#include <Foundation/Communication/Telemetry.h>
#include <Inspector/MainWindow.moc.h>
#include <QApplication>

class ezInspectorApp : public ezApplication
{
public:
  virtual void BeforeEngineInit() EZ_OVERRIDE
  {
  }

  virtual void AfterEngineInit() EZ_OVERRIDE
  {
  }

  virtual void BeforeEngineShutdown() EZ_OVERRIDE
  {
  }

  virtual void AfterEngineShutdown() EZ_OVERRIDE
  {
  }

  virtual ApplicationExecution Run() EZ_OVERRIDE
  {
    int iArgs = GetArgumentCount();
    char** cArgs = (char**) GetArgumentsArray();

    QApplication app(iArgs, cArgs);
    app.setApplicationName("ezInspector");
    ezMainWindow MainWindow;

    ezTelemetry::AcceptMessagesForSystem('LOG', true, ezMainWindow::ProcessTelemetry_Log, &MainWindow);
    ezTelemetry::AcceptMessagesForSystem('MEM', true, ezMainWindow::ProcessTelemetry_Memory, &MainWindow);
    ezTelemetry::AcceptMessagesForSystem('APP', true, ezMainWindow::ProcessTelemetry_General, &MainWindow);

    ezTelemetry::ConnectToServer();

    MainWindow.show();
    SetReturnCode(app.exec());

    ezTelemetry::CloseConnection();

    return ezApplication::Quit;
  }

};

EZ_APPLICATION_ENTRY_POINT(ezInspectorApp);


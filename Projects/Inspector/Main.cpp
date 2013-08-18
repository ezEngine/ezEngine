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

    ezTelemetry::AcceptMessagesForSystem('LOG', true);
    ezTelemetry::AcceptMessagesForSystem('MEM', true);

    ezTelemetry::ConnectToServer();

    QApplication app(iArgs, cArgs);
    app.setApplicationName("ezInspector");
    ezMainWindow mainWindow;
    mainWindow.show();
    SetReturnCode(app.exec());

    ezTelemetry::CloseConnection();

    return ezApplication::Quit;
  }

};

EZ_APPLICATION_ENTRY_POINT(ezInspectorApp);


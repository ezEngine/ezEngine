#include <Core/Application/Application.h>
#include <Foundation/Communication/Telemetry.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/LogWidget.moc.h>
#include <Inspector/GeneralWidget.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <QApplication>
#include <qstylefactory.h>

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

  virtual ApplicationExecution Run() EZ_OVERRIDE
  {
    int iArgs = GetArgumentCount();
    char** cArgs = (char**) GetArgumentsArray();

    QApplication app(iArgs, cArgs);
    app.setApplicationName("ezInspector");

    SetStyleSheet();

    ezMainWindow MainWindow;

    QWidget* pCenter = new QWidget();
    pCenter->setMinimumHeight(0);
    pCenter->setMaximumHeight(0);
    pCenter->setFixedHeight(0);
    pCenter->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    MainWindow.setCentralWidget(pCenter);

    ezLogWidget* pLogWidget = new ezLogWidget(&MainWindow);
    ezMemoryWidget* pMemoryWidget = new ezMemoryWidget(&MainWindow);
    ezGeneralWidget* pGeneralWidget = new ezGeneralWidget(&MainWindow);

    MainWindow.addDockWidget(Qt::BottomDockWidgetArea, pGeneralWidget);
    MainWindow.splitDockWidget(pGeneralWidget, pLogWidget, Qt::Horizontal);
    MainWindow.splitDockWidget(pGeneralWidget, pMemoryWidget, Qt::Vertical);

    //pCenter->setMinimumHeight(0);
    //pCenter->setMaximumHeight(100);
    //pCenter->setFixedHeight(100);
    //pCenter->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    //pCenter->resize(pCenter->width(), 100);


    ezTelemetry::AcceptMessagesForSystem('LOG', true, ezLogWidget::ProcessTelemetry_Log, pLogWidget);
    ezTelemetry::AcceptMessagesForSystem('MEM', true, ezMemoryWidget::ProcessTelemetry_Memory, pMemoryWidget);
    ezTelemetry::AcceptMessagesForSystem('APP', true, ezGeneralWidget::ProcessTelemetry_General, pGeneralWidget);

    ezTelemetry::ConnectToServer();

    //MainWindow.LoadLayout("layout.qt");

    MainWindow.show();
    SetReturnCode(app.exec());

    ezTelemetry::CloseConnection();

    QByteArray ba = MainWindow.saveState();

    MainWindow.SaveLayout("layout.qt");

    return ezApplication::Quit;
  }

};

EZ_APPLICATION_ENTRY_POINT(ezInspectorApp);


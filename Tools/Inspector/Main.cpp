#include <PCH.h>
#include <Core/Application/Application.h>
#include <Foundation/Communication/Telemetry.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/LogWidget.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/TimeWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/ReflectionWidget.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Inspector/GlobalEventsWidget.moc.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <Inspector/ResourceWidget.moc.h>
#include <QApplication>
#include <qstylefactory.h>
#include <QSettings>

class ezInspectorApp : public ezApplication
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

    QApplication app(iArgs, cArgs);
    QCoreApplication::setOrganizationDomain("www.ezengine.net");
    QCoreApplication::setOrganizationName("ezEngine Project");
    QCoreApplication::setApplicationName("ezInspector");
    QCoreApplication::setApplicationVersion("1.0.0");

    SetStyleSheet();

    ezMainWindow MainWindow;

    ezTelemetry::AcceptMessagesForSystem('CVAR', true, ezCVarsWidget::ProcessTelemetry, nullptr);
    ezTelemetry::AcceptMessagesForSystem('LOG', true, ezLogWidget::ProcessTelemetry, nullptr);
    ezTelemetry::AcceptMessagesForSystem('MEM', true, ezMemoryWidget::ProcessTelemetry, nullptr);
    ezTelemetry::AcceptMessagesForSystem('TIME', true, ezTimeWidget::ProcessTelemetry, nullptr);
    ezTelemetry::AcceptMessagesForSystem('APP', true, ezMainWindow::ProcessTelemetry, nullptr);
    ezTelemetry::AcceptMessagesForSystem('FILE', true, ezFileWidget::ProcessTelemetry, nullptr);
    ezTelemetry::AcceptMessagesForSystem('INPT', true, ezInputWidget::ProcessTelemetry, nullptr);
    ezTelemetry::AcceptMessagesForSystem('STRT', true, ezSubsystemsWidget::ProcessTelemetry, nullptr);
    ezTelemetry::AcceptMessagesForSystem('STAT', true, ezMainWindow::ProcessTelemetry, nullptr);
    ezTelemetry::AcceptMessagesForSystem('PLUG', true, ezPluginsWidget::ProcessTelemetry, nullptr);
    ezTelemetry::AcceptMessagesForSystem('EVNT', true, ezGlobalEventsWidget::ProcessTelemetry, nullptr);
    ezTelemetry::AcceptMessagesForSystem('RFLC', true, ezReflectionWidget::ProcessTelemetry, nullptr);
    ezTelemetry::AcceptMessagesForSystem('TRAN', true, ezDataWidget::ProcessTelemetry, nullptr);
    ezTelemetry::AcceptMessagesForSystem('RESM', true, ezResourceWidget::ProcessTelemetry, nullptr);
    
    QSettings Settings;
    const QString sServer = Settings.value("LastConnection", QLatin1String("localhost:1040")).toString();

    ezTelemetry::ConnectToServer(sServer.toUtf8().data());

    MainWindow.show();
    SetReturnCode(app.exec());

    ezTelemetry::CloseConnection();



    return ezApplication::Quit;
  }

};

EZ_APPLICATION_ENTRY_POINT(ezInspectorApp);


#include <PCH.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/TimeWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/ReflectionWidget.moc.h>
#include <Inspector/LogWidget.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Inspector/GlobalEventsWidget.moc.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <Inspector/ResourceWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <qinputdialog.h>
#include <QSettings>

void ezQtMainWindow::on_ActionShowWindowLog_triggered()
{
  ezQtLogWidget::s_pWidget->setVisible(ActionShowWindowLog->isChecked());
  ezQtLogWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowMemory_triggered()
{
  ezQtMemoryWidget::s_pWidget->setVisible(ActionShowWindowMemory->isChecked());
  ezQtMemoryWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowTime_triggered()
{
  ezQtTimeWidget::s_pWidget->setVisible(ActionShowWindowTime->isChecked());
  ezQtTimeWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowInput_triggered()
{
  ezQtInputWidget::s_pWidget->setVisible(ActionShowWindowInput->isChecked());
  ezQtInputWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowCVar_triggered()
{
  ezQtCVarsWidget::s_pWidget->setVisible(ActionShowWindowCVar->isChecked());
  ezQtCVarsWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowReflection_triggered()
{
  ezQtReflectionWidget::s_pWidget->setVisible(ActionShowWindowReflection->isChecked());
  ezQtReflectionWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowSubsystems_triggered()
{
  ezQtSubsystemsWidget::s_pWidget->setVisible(ActionShowWindowSubsystems->isChecked());
  ezQtSubsystemsWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowPlugins_triggered()
{
  ezQtPluginsWidget::s_pWidget->setVisible(ActionShowWindowPlugins->isChecked());
  ezQtPluginsWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowFile_triggered()
{
  ezQtFileWidget::s_pWidget->setVisible(ActionShowWindowFile->isChecked());
  ezQtFileWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowGlobalEvents_triggered()
{
  ezQtGlobalEventsWidget::s_pWidget->setVisible(ActionShowWindowGlobalEvents->isChecked());
  ezQtGlobalEventsWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowData_triggered()
{
  ezQtDataWidget::s_pWidget->setVisible(ActionShowWindowData->isChecked());
  ezQtDataWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowResource_triggered()
{
  ezQtResourceWidget::s_pWidget->setVisible(ActionShowWindowResource->isChecked());
  ezQtResourceWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionOnTopWhenConnected_triggered()
{
  SetAlwaysOnTop(WhenConnected);
}

void ezQtMainWindow::on_ActionAlwaysOnTop_triggered()
{
  SetAlwaysOnTop(Always);
}

void ezQtMainWindow::on_ActionNeverOnTop_triggered()
{
  SetAlwaysOnTop(Never);
}

void ezQtMainWindow::on_ButtonConnect_clicked()
{
  QSettings Settings;
  const QString sServer = Settings.value("LastConnection", QLatin1String("localhost:1040")).toString();

  bool bOk = false;
  QString sRes = QInputDialog::getText(this, "Host", "Host Name or IP Address:\nDefault is 'localhost:1040'", QLineEdit::Normal, sServer, &bOk);

  if (!bOk)
    return;

  Settings.setValue("LastConnection", sRes);

  if (ezTelemetry::ConnectToServer(sRes.toUtf8().data()) == EZ_SUCCESS)
  {
  }
}



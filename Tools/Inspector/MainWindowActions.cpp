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

void ezMainWindow::on_ActionShowWindowLog_triggered()
{
  ezLogWidget::s_pWidget->setVisible(ActionShowWindowLog->isChecked());
  ezLogWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowMemory_triggered()
{
  ezMemoryWidget::s_pWidget->setVisible(ActionShowWindowMemory->isChecked());
  ezMemoryWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowTime_triggered()
{
  ezTimeWidget::s_pWidget->setVisible(ActionShowWindowTime->isChecked());
  ezTimeWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowInput_triggered()
{
  ezInputWidget::s_pWidget->setVisible(ActionShowWindowInput->isChecked());
  ezInputWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowCVar_triggered()
{
  ezCVarsWidget::s_pWidget->setVisible(ActionShowWindowCVar->isChecked());
  ezCVarsWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowReflection_triggered()
{
  ezReflectionWidget::s_pWidget->setVisible(ActionShowWindowReflection->isChecked());
  ezReflectionWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowSubsystems_triggered()
{
  ezSubsystemsWidget::s_pWidget->setVisible(ActionShowWindowSubsystems->isChecked());
  ezSubsystemsWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowPlugins_triggered()
{
  ezPluginsWidget::s_pWidget->setVisible(ActionShowWindowPlugins->isChecked());
  ezPluginsWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowFile_triggered()
{
  ezFileWidget::s_pWidget->setVisible(ActionShowWindowFile->isChecked());
  ezFileWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowGlobalEvents_triggered()
{
  ezGlobalEventsWidget::s_pWidget->setVisible(ActionShowWindowGlobalEvents->isChecked());
  ezGlobalEventsWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowData_triggered()
{
  ezDataWidget::s_pWidget->setVisible(ActionShowWindowData->isChecked());
  ezDataWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionShowWindowResource_triggered()
{
  ezResourceWidget::s_pWidget->setVisible(ActionShowWindowResource->isChecked());
  ezResourceWidget::s_pWidget->raise();
}

void ezMainWindow::on_ActionOnTopWhenConnected_triggered()
{
  SetAlwaysOnTop(WhenConnected);
}

void ezMainWindow::on_ActionAlwaysOnTop_triggered()
{
  SetAlwaysOnTop(Always);
}

void ezMainWindow::on_ActionNeverOnTop_triggered()
{
  SetAlwaysOnTop(Never);
}

void ezMainWindow::on_ButtonConnect_clicked()
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



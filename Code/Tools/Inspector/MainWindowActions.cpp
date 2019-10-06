#include <InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/GlobalEventsWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Inspector/ReflectionWidget.moc.h>
#include <Inspector/ResourceWidget.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/TimeWidget.moc.h>

void ezQtMainWindow::on_ActionShowWindowLog_triggered()
{
  ezQtLogDockWidget::s_pWidget->toggleView(ActionShowWindowLog->isChecked());
  ezQtLogDockWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowMemory_triggered()
{
  ezQtMemoryWidget::s_pWidget->toggleView(ActionShowWindowMemory->isChecked());
  ezQtMemoryWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowTime_triggered()
{
  ezQtTimeWidget::s_pWidget->toggleView(ActionShowWindowTime->isChecked());
  ezQtTimeWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowInput_triggered()
{
  ezQtInputWidget::s_pWidget->toggleView(ActionShowWindowInput->isChecked());
  ezQtInputWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowCVar_triggered()
{
  ezQtCVarsWidget::s_pWidget->toggleView(ActionShowWindowCVar->isChecked());
  ezQtCVarsWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowReflection_triggered()
{
  ezQtReflectionWidget::s_pWidget->toggleView(ActionShowWindowReflection->isChecked());
  ezQtReflectionWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowSubsystems_triggered()
{
  ezQtSubsystemsWidget::s_pWidget->toggleView(ActionShowWindowSubsystems->isChecked());
  ezQtSubsystemsWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowPlugins_triggered()
{
  ezQtPluginsWidget::s_pWidget->toggleView(ActionShowWindowPlugins->isChecked());
  ezQtPluginsWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowFile_triggered()
{
  ezQtFileWidget::s_pWidget->toggleView(ActionShowWindowFile->isChecked());
  ezQtFileWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowGlobalEvents_triggered()
{
  ezQtGlobalEventsWidget::s_pWidget->toggleView(ActionShowWindowGlobalEvents->isChecked());
  ezQtGlobalEventsWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowData_triggered()
{
  ezQtDataWidget::s_pWidget->toggleView(ActionShowWindowData->isChecked());
  ezQtDataWidget::s_pWidget->raise();
}

void ezQtMainWindow::on_ActionShowWindowResource_triggered()
{
  ezQtResourceWidget::s_pWidget->toggleView(ActionShowWindowResource->isChecked());
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

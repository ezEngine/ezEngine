#include <Inspector/LogWidget.moc.h>
#include <qlistwidget.h>
#include <Foundation/Communication/Telemetry.h>
#include <MainWindow.moc.h>

ezLogWidget* ezLogWidget::s_pWidget = NULL;

ezLogWidget::ezLogWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

}

void ezLogWidget::on_ButtonClearLog_clicked()
{
  ListLog->clear();
}

void ezLogWidget::UpdateStats()
{
}


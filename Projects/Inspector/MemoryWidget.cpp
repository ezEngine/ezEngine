#include <Inspector/MemoryWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>

ezMemoryWidget* ezMemoryWidget::s_pWidget = NULL;

ezMemoryWidget::ezMemoryWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

}

void ezMemoryWidget::UpdateStats()
{
  if (!ezTelemetry::IsConnectedToServer())
  {
    LabelNumAllocs->setText(QString::fromUtf8("Allocations: N/A"));
    LabelNumDeallocs->setText(QString::fromUtf8("Deallocations: N/A"));
    LabelNumLiveAllocs->setText(QString::fromUtf8("Live Allocs: N/A"));
    LabelUsedMemory->setText(QString::fromUtf8("Used Memory: N/A"));
    LabelMemoryOverhead->setText(QString::fromUtf8("Overhead: N/A"));
  }
}


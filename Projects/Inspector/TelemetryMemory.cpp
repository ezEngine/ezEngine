#include <Inspector/MainWindow.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <qlistwidget.h>
#include <qinputdialog.h>

void ezMainWindow::ProcessTelemetry_Memory(void* pPassThrough)
{
  ezMainWindow* pWindow = (ezMainWindow*) pPassThrough;

  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage('MEM', Msg) == EZ_SUCCESS)
  {
    ezIAllocator::Stats MemStat;
    Msg.GetReader().ReadBytes(&MemStat, sizeof(ezIAllocator::Stats));

    pWindow->LabelNumAllocs->setText(QString::fromUtf8("Allocations: %1").arg(MemStat.m_uiNumAllocations));
    pWindow->LabelNumDeallocs->setText(QString::fromUtf8("Deallocations: %1").arg(MemStat.m_uiNumDeallocations));
    pWindow->LabelNumLiveAllocs->setText(QString::fromUtf8("Live Allocs: %1").arg(MemStat.m_uiNumLiveAllocations));

    if (MemStat.m_uiUsedMemorySize < 1024)
      pWindow->LabelUsedMemory->setText(QString::fromUtf8("Used Memory: %1 Byte").arg(MemStat.m_uiUsedMemorySize));
    else
      if (MemStat.m_uiUsedMemorySize < 1024 * 1024)
        pWindow->LabelUsedMemory->setText(QString::fromUtf8("Used Memory: %1 KB").arg(MemStat.m_uiUsedMemorySize / 1024));
      else
        pWindow->LabelUsedMemory->setText(QString::fromUtf8("Used Memory: %1 MB").arg(MemStat.m_uiUsedMemorySize / 1024 / 1024));

    const ezUInt32 uiOverhead = MemStat.m_uiUsedMemorySize - MemStat.m_uiAllocationSize;

    if (uiOverhead < 1024)
      pWindow->LabelMemoryOverhead->setText(QString::fromUtf8("Overhead: %1 Byte").arg(uiOverhead));
    else
      if (uiOverhead < 1024 * 1024)
        pWindow->LabelMemoryOverhead->setText(QString::fromUtf8("Overhead: %1 KB").arg(uiOverhead / 1024));
      else
        pWindow->LabelMemoryOverhead->setText(QString::fromUtf8("Overhead: %1 MB").arg((uiOverhead) / 1024 / 1024));

  }

}


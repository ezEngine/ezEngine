#include <Inspector/MemoryWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <QGraphicsView>
#include <QGraphicsPathItem>

ezMemoryWidget* ezMemoryWidget::s_pWidget = NULL;

ezMemoryWidget::ezMemoryWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

  m_pPath = m_Scene.addPath (QPainterPath (), QPen (QBrush (QColor (71, 137, 17)), 0 ));

  QTransform t = UsedMemoryView->transform ();
  t.scale (1, -1);
  UsedMemoryView->setTransform (t);

  UsedMemoryView->setScene(&m_Scene);

  UsedMemoryView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  UsedMemoryView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  UsedMemoryView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  //UsedMemoryView->setMaximumHeight(100);

  ResetStats();
}

void ezMemoryWidget::ResetStats()
{
  m_uiDropOne = 0;
  m_uiMinUsedMemory = 0xFFFFFFFF;
  m_uiMaxUsedMemory = 0;
  m_uiMaxUsedMemoryRecently = 0;
  m_UsedMemory.Clear();
  m_uiMaxSamples = 300;
}

void ezMemoryWidget::UpdateStats()
{
  if (!ezTelemetry::IsConnectedToServer())
  {
    LabelNumAllocs->setText(QString::fromUtf8("Alloc Counter: N/A"));
    LabelNumLiveAllocs->setText(QString::fromUtf8("Live Allocs: N/A"));
    
    LabelCurMemory->setText(QString::fromUtf8("Cur: N/A"));
    LabelMaxMemory->setText(QString::fromUtf8("Max: N/A"));
    LabelMinMemory->setText(QString::fromUtf8("Min: N/A"));

    return;
  }

  if (ezSystemTime::Now() - s_pWidget->m_LastUsedMemoryStored > ezTime::MilliSeconds(200))
  {
    s_pWidget->m_LastUsedMemoryStored = ezSystemTime::Now();

    s_pWidget->m_UsedMemory.PushBack(s_pWidget->m_uiMaxUsedMemoryRecently);

    s_pWidget->m_uiMaxUsedMemoryRecently = 0;

    ++s_pWidget->m_uiDropOne;

    if (s_pWidget->m_uiDropOne >= 5)
    {
      s_pWidget->m_uiDropOne = 0;
      s_pWidget->m_UsedMemory.PopFront();
    }
  }
  else
    return;

  if (!m_UsedMemory.IsEmpty())
  {
    QPainterPath pp;

    if (m_UsedMemory.GetCount() > m_uiMaxSamples)
      m_UsedMemory.PopFront(m_UsedMemory.GetCount() - m_uiMaxSamples);

    pp.moveTo (QPointF (0.0f, m_UsedMemory[0]));
    m_uiMinUsedMemory = m_UsedMemory[0];

    for (ezUInt32 i = 1; i < m_UsedMemory.GetCount(); ++i)
    {
      pp.lineTo (QPointF (i, m_UsedMemory[i]));
      m_uiMinUsedMemory = ezMath::Min(m_uiMinUsedMemory, m_UsedMemory[i]);
    }

    m_pPath->setPath (pp);

    UsedMemoryView->setSceneRect (QRectF (0, m_uiMinUsedMemory, m_UsedMemory.GetCount() - 1, m_uiMaxUsedMemory));
    UsedMemoryView->fitInView    (QRectF (0, m_uiMinUsedMemory, m_UsedMemory.GetCount() - 1, m_uiMaxUsedMemory));

    ezStringBuilder s;

    s.Format("Min: %.2f MB", m_uiMinUsedMemory / 1024.0 / 1024.0);
    LabelMinMemory->setText(QString::fromUtf8(s.GetData()));

    s.Format("<p>Recent Minimum Memory Usage:<br>%.2f GB<br>%.2f MB<br>%.2f KB<br>%llu Byte</p>", 
      m_uiMinUsedMemory / 1024.0 / 1024.0 / 1024.0,
      m_uiMinUsedMemory / 1024.0 / 1024.0,
      m_uiMinUsedMemory / 1024.0,
      m_uiMinUsedMemory);
    LabelMinMemory->setToolTip(QString::fromUtf8(s.GetData()));

    s.Format("Max: %.2f MB", m_uiMaxUsedMemory / 1024.0 / 1024.0);
    LabelMaxMemory->setText(QString::fromUtf8(s.GetData()));

    s.Format("<p>Recent Maximum Memory Usage:<br>%.2f GB<br>%.2f MB<br>%.2f KB<br>%llu Byte</p>", 
      m_uiMaxUsedMemory / 1024.0 / 1024.0 / 1024.0,
      m_uiMaxUsedMemory / 1024.0 / 1024.0,
      m_uiMaxUsedMemory / 1024.0,
      m_uiMaxUsedMemory);
    LabelMaxMemory->setToolTip(QString::fromUtf8(s.GetData()));

    const ezUInt64 uiCurUsedMemory = m_UsedMemory.PeekBack();

    s.Format("Cur: %.2f MB", uiCurUsedMemory / 1024.0 / 1024.0);
    LabelCurMemory->setText(QString::fromUtf8(s.GetData()));

    s.Format("<p>Current Memory Usage:<br>%.2f GB<br>%.2f MB<br>%.2f KB<br>%llu Byte</p>", 
      uiCurUsedMemory / 1024.0 / 1024.0 / 1024.0,
      uiCurUsedMemory / 1024.0 / 1024.0,
      uiCurUsedMemory / 1024.0,
      uiCurUsedMemory);
    LabelCurMemory->setToolTip(QString::fromUtf8(s.GetData()));

    s.Format("Allocs: %llu", m_uiLiveAllocs);
    LabelNumLiveAllocs->setText(QString::fromUtf8(s.GetData()));

    s.Format("Counter: %llu / %llu", m_uiAllocs, m_uiDeallocs);
    LabelNumAllocs->setText(QString::fromUtf8(s.GetData()));

    s.Format("<p>Allocations: <b>%llu</b><br>Deallocations: <b>%llu</b></p>", m_uiAllocs, m_uiDeallocs);
    LabelNumAllocs->setToolTip(QString::fromUtf8(s.GetData()));
  }
}

void ezMemoryWidget::ProcessTelemetry_Memory(void* pPassThrough)
{
  if (s_pWidget == NULL)
    return;

  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage('MEM', Msg) == EZ_SUCCESS)
  {
    ezIAllocator::Stats MemStat;
    Msg.GetReader().ReadBytes(&MemStat, sizeof(ezIAllocator::Stats));

    s_pWidget->m_uiAllocs = MemStat.m_uiNumAllocations;
    s_pWidget->m_uiDeallocs = MemStat.m_uiNumDeallocations;
    s_pWidget->m_uiLiveAllocs = MemStat.m_uiNumLiveAllocations;

    s_pWidget->m_uiMaxUsedMemory = ezMath::Max(s_pWidget->m_uiMaxUsedMemory, MemStat.m_uiUsedMemorySize);
    s_pWidget->m_uiMaxUsedMemoryRecently = ezMath::Max(s_pWidget->m_uiMaxUsedMemoryRecently, MemStat.m_uiUsedMemorySize);
  }
}


#include <Inspector/MemoryWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <QGraphicsView>
#include <QGraphicsPathItem>

ezMemoryWidget* ezMemoryWidget::s_pWidget = NULL;

static QColor s_Colors[ezMemoryWidget::s_uiMaxColors] =
{
  QColor(255, 106,   0), // orange
  QColor(182, 255,   0), // lime green
  QColor(255,   0, 255), // pink
  QColor(  0, 148, 255), // light blue
  QColor(255,   0,   0), // red
  QColor(  0, 255, 255), // turqoise
  QColor(178,   0, 255), // purple
  QColor(  0,  38, 255), // dark blue
  QColor( 72,   0, 255), // lilac
};


ezMemoryWidget::ezMemoryWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

  for (ezUInt32 i = 0; i < s_uiMaxColors; ++i)
    m_pPath[i] = m_Scene.addPath (QPainterPath (), QPen (QBrush (s_Colors[i]), 0 ));

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
  m_AllocatorData.Clear();

  m_uiDropOne = 0;
  m_uiMaxSamples = 300;
  m_uiColorsUsed = 0;
  m_bAllocatorsChanged = true;

  ListAllocators->clear();
}

void ezMemoryWidget::UpdateStats()
{
  if (!ezTelemetry::IsConnectedToServer())
  {
    ListAllocators->setEnabled(false);

    LabelNumAllocs->setText(QString::fromUtf8("Alloc Counter: N/A"));
    LabelNumLiveAllocs->setText(QString::fromUtf8("Live Allocs: N/A"));
    
    LabelCurMemory->setText(QString::fromUtf8("Cur: N/A"));
    LabelMaxMemory->setText(QString::fromUtf8("Max: N/A"));
    LabelMinMemory->setText(QString::fromUtf8("Min: N/A"));

    return;
  }

  ListAllocators->setEnabled(true);

  if (m_bAllocatorsChanged)
  {
    m_bAllocatorsChanged = false;

    ListAllocators->blockSignals(true);
    ListAllocators->clear();

    for (ezMap<ezString, ezMemoryWidget::AllocatorData>::Iterator it = m_AllocatorData.GetIterator(); it.IsValid(); ++it)
    {
      ListAllocators->addItem(it.Key().GetData());

      QListWidgetItem* pItem = ListAllocators->item(ListAllocators->count() - 1);
      pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);
      pItem->setCheckState (it.Value().m_bDisplay ? Qt::Checked : Qt::Unchecked);

      pItem->setTextColor(s_Colors[it.Value().m_iColor % s_uiMaxColors]);
    }

    ListAllocators->blockSignals(false);
  }

  if (ezSystemTime::Now() - s_pWidget->m_LastUsedMemoryStored > ezTime::MilliSeconds(200))
  {
    m_LastUsedMemoryStored = ezSystemTime::Now();

    ++m_uiDropOne;

    for (ezMap<ezString, ezMemoryWidget::AllocatorData>::Iterator it = m_AllocatorData.GetIterator(); it.IsValid(); ++it)
    {
      it.Value().m_UsedMemory.PushBack(it.Value().m_uiMaxUsedMemoryRecently);

      it.Value().m_uiMaxUsedMemoryRecently = 0;

      if (m_uiDropOne >= 5)
        it.Value().m_UsedMemory.PopFront();
    }

    if (m_uiDropOne >= 5)
      m_uiDropOne = 0;
  }
  else
    return;

  QPainterPath pp[s_uiMaxColors];

  ezUInt32 uiMaxSamples = 0;

  ezUInt64 uiUsedMemory = 0;
  ezUInt64 uiLiveAllocs = 0;
  ezUInt64 uiAllocs = 0;
  ezUInt64 uiDeallocs = 0;
  ezUInt64 uiMinUsedMemory = 0xFFFFFFFF;
  ezUInt64 uiMaxUsedMemory = 0;

  for (ezMap<ezString, AllocatorData>::Iterator it = s_pWidget->m_AllocatorData.GetIterator(); it.IsValid(); ++it)
  {
    if (!it.Value().m_UsedMemory.IsEmpty() && it.Value().m_bDisplay)
    {
      const ezUInt32 uiColorPath = it.Value().m_iColor % s_uiMaxColors;

      uiUsedMemory += it.Value().m_UsedMemory.PeekBack();
      uiLiveAllocs += it.Value().m_uiLiveAllocs;
      uiAllocs     += it.Value().m_uiAllocs;
      uiDeallocs   += it.Value().m_uiDeallocs;

      uiMaxSamples = ezMath::Max(uiMaxSamples, it.Value().m_UsedMemory.GetCount());

      if (it.Value().m_UsedMemory.GetCount() > m_uiMaxSamples)
        it.Value().m_UsedMemory.PopFront(it.Value().m_UsedMemory.GetCount() - m_uiMaxSamples);

      pp[uiColorPath].moveTo (QPointF (0.0f, it.Value().m_UsedMemory[0]));
      uiMinUsedMemory = ezMath::Min(uiMinUsedMemory, it.Value().m_UsedMemory[0]);

      for (ezUInt32 i = 1; i < it.Value().m_UsedMemory.GetCount(); ++i)
      {
        pp[uiColorPath].lineTo (QPointF (i, it.Value().m_UsedMemory[i]));
        uiMinUsedMemory = ezMath::Min(uiMinUsedMemory, it.Value().m_UsedMemory[i]);
      }

      uiMaxUsedMemory = ezMath::Max(uiMaxUsedMemory, it.Value().m_uiMaxUsedMemory);
    }
  }

  for (ezUInt32 i = 0; i < s_uiMaxColors; ++i)
    m_pPath[i]->setPath (pp[i]);

  {
    UsedMemoryView->setSceneRect (QRectF (0, uiMinUsedMemory, uiMaxSamples - 1, uiMaxUsedMemory));
    UsedMemoryView->fitInView    (QRectF (0, uiMinUsedMemory, uiMaxSamples - 1, uiMaxUsedMemory));

    ezStringBuilder s;

    s.Format("Min: %.2f MB", uiMinUsedMemory / 1024.0 / 1024.0);
    LabelMinMemory->setText(QString::fromUtf8(s.GetData()));

    s.Format("<p>Recent Minimum Memory Usage:<br>%.2f GB<br>%.2f MB<br>%.2f KB<br>%llu Byte</p>", 
      uiMinUsedMemory / 1024.0 / 1024.0 / 1024.0,
      uiMinUsedMemory / 1024.0 / 1024.0,
      uiMinUsedMemory / 1024.0,
      uiMinUsedMemory);
    LabelMinMemory->setToolTip(QString::fromUtf8(s.GetData()));

    s.Format("Max: %.2f MB", uiMaxUsedMemory / 1024.0 / 1024.0);
    LabelMaxMemory->setText(QString::fromUtf8(s.GetData()));

    s.Format("<p>Recent Maximum Memory Usage:<br>%.2f GB<br>%.2f MB<br>%.2f KB<br>%llu Byte</p>", 
      uiMaxUsedMemory / 1024.0 / 1024.0 / 1024.0,
      uiMaxUsedMemory / 1024.0 / 1024.0,
      uiMaxUsedMemory / 1024.0,
      uiMaxUsedMemory);
    LabelMaxMemory->setToolTip(QString::fromUtf8(s.GetData()));

    const ezUInt64 uiCurUsedMemory = uiUsedMemory;

    s.Format("Cur: %.2f MB", uiCurUsedMemory / 1024.0 / 1024.0);
    LabelCurMemory->setText(QString::fromUtf8(s.GetData()));

    s.Format("<p>Current Memory Usage:<br>%.2f GB<br>%.2f MB<br>%.2f KB<br>%llu Byte</p>", 
      uiCurUsedMemory / 1024.0 / 1024.0 / 1024.0,
      uiCurUsedMemory / 1024.0 / 1024.0,
      uiCurUsedMemory / 1024.0,
      uiCurUsedMemory);
    LabelCurMemory->setToolTip(QString::fromUtf8(s.GetData()));

    s.Format("Allocs: %llu", uiLiveAllocs);
    LabelNumLiveAllocs->setText(QString::fromUtf8(s.GetData()));

    s.Format("Counter: %llu / %llu", uiAllocs, uiDeallocs);
    LabelNumAllocs->setText(QString::fromUtf8(s.GetData()));

    s.Format("<p>Allocations: <b>%llu</b><br>Deallocations: <b>%llu</b></p>", uiAllocs, uiDeallocs);
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
    ezString sAllocatorName;

    ezIAllocator::Stats MemStat;
    Msg.GetReader() >> sAllocatorName;
    Msg.GetReader() >> MemStat;

    AllocatorData& ad = s_pWidget->m_AllocatorData[sAllocatorName];

    if (ad.m_iColor < 0)
    {
      ad.m_iColor = s_pWidget->m_uiColorsUsed;
      ++s_pWidget->m_uiColorsUsed;
      s_pWidget->m_bAllocatorsChanged = true;
    }

    ad.m_uiAllocs = MemStat.m_uiNumAllocations;
    ad.m_uiDeallocs = MemStat.m_uiNumDeallocations;
    ad.m_uiLiveAllocs = MemStat.m_uiNumLiveAllocations;
    ad.m_uiMaxUsedMemoryRecently = ezMath::Max(ad.m_uiMaxUsedMemoryRecently, MemStat.m_uiUsedMemorySize);
    ad.m_uiMaxUsedMemory = ezMath::Max(ad.m_uiMaxUsedMemory, MemStat.m_uiUsedMemorySize);
  }
}

void ezMemoryWidget::on_ListAllocators_itemChanged(QListWidgetItem* item)
{
  m_AllocatorData[item->text().toUtf8().data()].m_bDisplay = (item->checkState() == Qt::Checked);
}


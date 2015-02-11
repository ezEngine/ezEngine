#include <PCH.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <QGraphicsView>
#include <QGraphicsPathItem>

ezMemoryWidget* ezMemoryWidget::s_pWidget = nullptr;

static QColor s_Colors[ezMemoryWidget::s_uiMaxColors] =
{
  QColor(255, 106,   0), // orange
  QColor(182, 255,   0), // lime green
  QColor(255,   0, 255), // pink
  QColor(  0, 148, 255), // light blue
  QColor(255,   0,   0), // red
  QColor(  0, 255, 255), // turquoise
  QColor(178,   0, 255), // purple
  QColor(  0,  38, 255), // dark blue
  QColor( 72,   0, 255), // lilac
};

void FormatSize(ezStringBuilder& s, const char* szPrefix, ezUInt64 uiSize)
{
  if (uiSize < 1024)
    s.Format("%s%llu Bytes", szPrefix, uiSize);
  else if (uiSize < 1024 * 1024)
    s.Format("%s%.1f KB", szPrefix, uiSize / 1024.0);
  else if (uiSize < 1024 * 1024 * 1024)
    s.Format("%s%.2f MB", szPrefix, uiSize / 1024.0 / 1024.0);
  else
    s.Format("%s%.2f GB", szPrefix, uiSize / 1024.0 / 1024.0 / 1024.0);
}

ezMemoryWidget::ezMemoryWidget(QWidget* parent) : QDockWidget (parent)
{
  s_pWidget = this;

  setupUi (this);

  ComboTimeframe->blockSignals(true);
  ComboTimeframe->addItem("Timeframe: 1 minute");
  ComboTimeframe->addItem("Timeframe: 2 minutes");
  ComboTimeframe->addItem("Timeframe: 3 minutes");
  ComboTimeframe->addItem("Timeframe: 4 minutes");
  ComboTimeframe->addItem("Timeframe: 5 minutes");
  ComboTimeframe->addItem("Timeframe: 6 minutes");
  ComboTimeframe->addItem("Timeframe: 7 minutes");
  ComboTimeframe->addItem("Timeframe: 8 minutes");
  ComboTimeframe->addItem("Timeframe: 9 minutes");
  ComboTimeframe->addItem("Timeframe: 10 minutes");
  ComboTimeframe->setCurrentIndex(0);
  ComboTimeframe->blockSignals(false);

  m_pPathMax = m_Scene.addPath (QPainterPath (), QPen (QBrush (QColor(255, 255, 255)), 0 ));

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

  m_uiMaxSamples = 3000;
  m_uiDisplaySamples = 5 * 60; // 5 samples per second, 60 seconds
  m_uiColorsUsed = 1;
  m_bAllocatorsChanged = true;

  m_Accu = AllocatorData();

  ListAllocators->clear();
}

void ezMemoryWidget::UpdateStats()
{
  if (!isVisible())
    return;

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

    {
      ListAllocators->addItem("<Accumulated>");

      m_Accu.m_pListItem = ListAllocators->item(ListAllocators->count() - 1);
      m_Accu.m_pListItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
      m_Accu.m_pListItem->setCheckState (m_Accu.m_bDisplay ? Qt::Checked : Qt::Unchecked);
      m_Accu.m_pListItem->setData(Qt::UserRole, QString("<Accumulated>"));

      m_Accu.m_pListItem->setTextColor(QColor(255, 255, 255));
    }

    for (ezMap<ezString, ezMemoryWidget::AllocatorData>::Iterator it = m_AllocatorData.GetIterator(); it.IsValid(); ++it)
    {
      ListAllocators->addItem(it.Key().GetData());

      QListWidgetItem* pItem = ListAllocators->item(ListAllocators->count() - 1);
      pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
      pItem->setCheckState (it.Value().m_bDisplay ? Qt::Checked : Qt::Unchecked);
      pItem->setData(Qt::UserRole, QString(it.Key().GetData()));

      pItem->setTextColor(s_Colors[it.Value().m_iColor % s_uiMaxColors]);

      it.Value().m_pListItem = pItem;
    }

    ListAllocators->blockSignals(false);
  }

  // once a second update the display of the allocators in the list
  if (ezTime::Now() - m_LastUpdatedAllocatorList > ezTime::Seconds(1))
  {
    m_LastUpdatedAllocatorList = ezTime::Now();

    for (ezMap<ezString, ezMemoryWidget::AllocatorData>::Iterator it = m_AllocatorData.GetIterator(); it.IsValid(); ++it)
    {
      if (!it.Value().m_pListItem || it.Value().m_UsedMemory.IsEmpty())
        continue;

      ezStringBuilder sSize;
      FormatSize(sSize, "", it.Value().m_UsedMemory.PeekBack());

      ezStringBuilder sMaxSize;
      FormatSize(sMaxSize, "", it.Value().m_uiMaxUsedMemory);

      ezStringBuilder sText = it.Key().GetData();
      sText.AppendFormat(" [%s]", sSize.GetData());

      ezStringBuilder sTooltip;
      sTooltip.Format("<p>Current Memory Used: <b>%s</b><br>Max Memory Used: <b>%s</b><br>Live Allocations: <b>%llu</b><br>Allocations: <b>%llu</b><br>Deallocations: <b>%llu</b><br>",
        sSize.GetData(), sMaxSize.GetData(), it.Value().m_uiLiveAllocs, it.Value().m_uiAllocs, it.Value().m_uiDeallocs);

      it.Value().m_pListItem->setText(sText.GetData());
      it.Value().m_pListItem->setToolTip(sTooltip.GetData());
    }

    if (m_Accu.m_pListItem && !m_Accu.m_UsedMemory.IsEmpty())
    {
      ezStringBuilder sSize;
      FormatSize(sSize, "", m_Accu.m_UsedMemory.PeekBack());

      ezStringBuilder sMaxSize;
      FormatSize(sMaxSize, "", m_Accu.m_uiMaxUsedMemory);

      ezStringBuilder sText = "<Accumulated>";
      sText.AppendFormat(" [%s]", sSize.GetData());

      ezStringBuilder sTooltip;
      sTooltip.Format("<p>Current Memory Used: <b>%s</b><br>Max Memory Used: <b>%s</b><br>Live Allocations: <b>%llu</b><br>Allocations: <b>%llu</b><br>Deallocations: <b>%llu</b><br>",
        sSize.GetData(), sMaxSize.GetData(), m_Accu.m_uiLiveAllocs, m_Accu.m_uiAllocs, m_Accu.m_uiDeallocs);

      m_Accu.m_pListItem->setText(sText.GetData());
      m_Accu.m_pListItem->setToolTip(sTooltip.GetData());
    }
  }

  if (ezTime::Now() - s_pWidget->m_LastUsedMemoryStored > ezTime::Milliseconds(200))
  {
    m_LastUsedMemoryStored = ezTime::Now();

    ezUInt64 uiSumMemory = 0;

    for (ezMap<ezString, ezMemoryWidget::AllocatorData>::Iterator it = m_AllocatorData.GetIterator(); it.IsValid(); ++it)
    {
      // sometimes no data arrives in time (game is too slow)
      // in this case simply assume the stats have not changed
      if (!it.Value().m_bReceivedData && !it.Value().m_UsedMemory.IsEmpty())
        it.Value().m_uiMaxUsedMemoryRecently = it.Value().m_UsedMemory.PeekBack();

      uiSumMemory += it.Value().m_uiMaxUsedMemoryRecently;

      it.Value().m_UsedMemory.PushBack(it.Value().m_uiMaxUsedMemoryRecently);

      it.Value().m_uiMaxUsedMemoryRecently = 0;
      it.Value().m_bReceivedData = false;
    }
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

  {
    m_Accu.m_UsedMemory.SetCount(m_uiDisplaySamples);

    for (ezUInt32 i = 0; i < m_uiDisplaySamples; ++i)
      m_Accu.m_UsedMemory[i] = 0;

    m_Accu.m_uiAllocs = 0;
    m_Accu.m_uiDeallocs = 0;
    m_Accu.m_uiLiveAllocs = 0;
    m_Accu.m_uiMaxUsedMemory = 0;
  }

  for (ezMap<ezString, AllocatorData>::Iterator it = s_pWidget->m_AllocatorData.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_UsedMemory.IsEmpty() || !it.Value().m_bDisplay)
      continue;

    ezUInt64 uiMinUsedMemoryThis = 0xFFFFFFFF;
    ezUInt64 uiMaxUsedMemoryThis = 0;

    const ezUInt32 uiColorPath = it.Value().m_iColor % s_uiMaxColors;

    uiUsedMemory += it.Value().m_UsedMemory.PeekBack();
    uiLiveAllocs += it.Value().m_uiLiveAllocs;
    uiAllocs     += it.Value().m_uiAllocs;
    uiDeallocs   += it.Value().m_uiDeallocs;

    if (it.Value().m_UsedMemory.GetCount() > m_uiMaxSamples)
      it.Value().m_UsedMemory.PopFront(it.Value().m_UsedMemory.GetCount() - m_uiMaxSamples);

    uiMaxSamples = ezMath::Max(uiMaxSamples, ezMath::Min(m_uiDisplaySamples, it.Value().m_UsedMemory.GetCount()));

    const ezUInt32 uiFirstSample = (it.Value().m_UsedMemory.GetCount() <= m_uiDisplaySamples) ? 0 : (it.Value().m_UsedMemory.GetCount() - m_uiDisplaySamples);
    const ezUInt32 uiStartPos    = (it.Value().m_UsedMemory.GetCount() >= m_uiDisplaySamples) ? 0 : (m_uiDisplaySamples - it.Value().m_UsedMemory.GetCount());

    pp[uiColorPath].moveTo (QPointF (uiStartPos, it.Value().m_UsedMemory[uiFirstSample]));
    uiMinUsedMemoryThis = ezMath::Min(uiMinUsedMemoryThis, it.Value().m_UsedMemory[uiFirstSample]);
    uiMaxUsedMemoryThis = ezMath::Max(uiMaxUsedMemoryThis, it.Value().m_UsedMemory[uiFirstSample]);

    {
      m_Accu.m_uiAllocs     += it.Value().m_uiAllocs;
      m_Accu.m_uiDeallocs   += it.Value().m_uiDeallocs;
      m_Accu.m_uiLiveAllocs += it.Value().m_uiLiveAllocs;

      m_Accu.m_UsedMemory[uiStartPos] += it.Value().m_UsedMemory[uiFirstSample];
      m_Accu.m_uiMaxUsedMemory += it.Value().m_uiMaxUsedMemory;
    }

    for (ezUInt32 i = uiFirstSample + 1; i < it.Value().m_UsedMemory.GetCount(); ++i)
    {
      pp[uiColorPath].lineTo (QPointF (uiStartPos + i - uiFirstSample, it.Value().m_UsedMemory[i]));

      uiMinUsedMemoryThis = ezMath::Min(uiMinUsedMemoryThis, it.Value().m_UsedMemory[i]);
      uiMaxUsedMemoryThis = ezMath::Max(uiMaxUsedMemoryThis, it.Value().m_UsedMemory[i]);

      m_Accu.m_UsedMemory[uiStartPos + i - uiFirstSample] += it.Value().m_UsedMemory[i];

      if (m_Accu.m_bDisplay)
        uiMaxUsedMemoryThis = ezMath::Max(uiMaxUsedMemoryThis, m_Accu.m_UsedMemory[uiStartPos + i - uiFirstSample]);
    }

    uiMinUsedMemory = ezMath::Min(uiMinUsedMemory, uiMinUsedMemoryThis);
    uiMaxUsedMemory = ezMath::Max(uiMaxUsedMemory, uiMaxUsedMemoryThis);
  }

  QPainterPath pMax;

  if (m_Accu.m_bDisplay)
  {
    pMax.moveTo (QPointF (0, m_Accu.m_UsedMemory[0]));

    for (ezUInt32 i = 1; i < m_Accu.m_UsedMemory.GetCount(); ++i)
      pMax.lineTo(QPointF(i, m_Accu.m_UsedMemory[i]));
  }

  m_pPathMax->setPath(pMax);

  for (ezUInt32 i = 0; i < s_uiMaxColors; ++i)
    m_pPath[i]->setPath(pp[i]);

  // round min and max to some power of two
  {
    uiMinUsedMemory = ezMath::PowerOfTwo_Floor(uiMinUsedMemory);
    uiMaxUsedMemory = ezMath::PowerOfTwo_Ceil(uiMaxUsedMemory);
  }

  {
    UsedMemoryView->setSceneRect (QRectF (0, uiMinUsedMemory, m_uiDisplaySamples - 1, uiMaxUsedMemory));
    UsedMemoryView->fitInView    (QRectF (0, uiMinUsedMemory, m_uiDisplaySamples - 1, uiMaxUsedMemory));

    ezStringBuilder s;
    
    FormatSize(s, "Min: ", uiMinUsedMemory);
    LabelMinMemory->setText(QString::fromUtf8(s.GetData()));

    s.Format("<p>Recent Minimum Memory Usage:<br>%.2f GB<br>%.2f MB<br>%.2f KB<br>%llu Byte</p>", 
      uiMinUsedMemory / 1024.0 / 1024.0 / 1024.0,
      uiMinUsedMemory / 1024.0 / 1024.0,
      uiMinUsedMemory / 1024.0,
      uiMinUsedMemory);
    LabelMinMemory->setToolTip(QString::fromUtf8(s.GetData()));

    FormatSize(s, "Max: ", uiMaxUsedMemory);
    LabelMaxMemory->setText(QString::fromUtf8(s.GetData()));

    s.Format("<p>Recent Maximum Memory Usage:<br>%.2f GB<br>%.2f MB<br>%.2f KB<br>%llu Byte</p>", 
      uiMaxUsedMemory / 1024.0 / 1024.0 / 1024.0,
      uiMaxUsedMemory / 1024.0 / 1024.0,
      uiMaxUsedMemory / 1024.0,
      uiMaxUsedMemory);
    LabelMaxMemory->setToolTip(QString::fromUtf8(s.GetData()));

    const ezUInt64 uiCurUsedMemory = uiUsedMemory;

    FormatSize(s, "Sum: ", uiCurUsedMemory);
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

void ezMemoryWidget::ProcessTelemetry(void* pUnuseed)
{
  if (s_pWidget == nullptr)
    return;

  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage('MEM', Msg) == EZ_SUCCESS)
  {
    ezString sAllocatorName;

    ezAllocatorBase::Stats MemStat;
    Msg.GetReader() >> sAllocatorName;
    Msg.GetReader() >> MemStat;

    AllocatorData& ad = s_pWidget->m_AllocatorData[sAllocatorName];
    ad.m_bReceivedData = true;

    if (ad.m_iColor < 0)
    {
      ad.m_iColor = s_pWidget->m_uiColorsUsed;
      ++s_pWidget->m_uiColorsUsed;
      s_pWidget->m_bAllocatorsChanged = true;
    }

    ad.m_uiAllocs = MemStat.m_uiNumAllocations;
    ad.m_uiDeallocs = MemStat.m_uiNumDeallocations;
    ad.m_uiLiveAllocs = MemStat.m_uiNumAllocations - MemStat.m_uiNumDeallocations;
    ad.m_uiMaxUsedMemoryRecently = ezMath::Max(ad.m_uiMaxUsedMemoryRecently, MemStat.m_uiAllocationSize);
    ad.m_uiMaxUsedMemory = ezMath::Max(ad.m_uiMaxUsedMemory, MemStat.m_uiAllocationSize);
  }
}

void ezMemoryWidget::on_ListAllocators_itemChanged(QListWidgetItem* item)
{
  if (item->data(Qt::UserRole).toString() == "<Accumulated>")
  {
    m_Accu.m_bDisplay = (item->checkState() == Qt::Checked);
    return;
  }

  m_AllocatorData[item->data(Qt::UserRole).toString().toUtf8().data()].m_bDisplay = (item->checkState() == Qt::Checked);
}

void ezMemoryWidget::on_ComboTimeframe_currentIndexChanged(int index)
{
  m_uiDisplaySamples = 5 * 60 * (index + 1); // 5 samples per second, 60 seconds
}
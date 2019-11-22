#include <InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <Inspector/MemoryWidget.moc.h>
#include <QGraphicsPathItem>
#include <QGraphicsView>
#include <QMenu>

ezQtMemoryWidget* ezQtMemoryWidget::s_pWidget = nullptr;

namespace MemoryWidgetDetail
{
  static QColor s_Colors[ezQtMemoryWidget::s_uiMaxColors] = {
    QColor(255, 106, 0),   // orange
    QColor(182, 255, 0),   // lime green
    QColor(255, 0, 255),   // pink
    QColor(0, 148, 255),   // light blue
    QColor(255, 0, 0),     // red
    QColor(0, 255, 255),   // turquoise
    QColor(217, 128, 255), // purple
    QColor(128, 147, 255), // dark blue
    QColor(164, 128, 255), // lilac
  };
}

void FormatSize(ezStringBuilder& s, const char* szPrefix, ezUInt64 uiSize)
{
  if (uiSize < 1024)
    s.Format("{0}{1} Bytes", szPrefix, uiSize);
  else if (uiSize < 1024 * 1024)
    s.Format("{0}{1} KB", szPrefix, ezArgF(uiSize / 1024.0, 1));
  else if (uiSize < 1024 * 1024 * 1024)
    s.Format("{0}{1} MB", szPrefix, ezArgF(uiSize / 1024.0 / 1024.0, 2));
  else
    s.Format("{0}{1} GB", szPrefix, ezArgF(uiSize / 1024.0 / 1024.0 / 1024.0, 2));
}

ezQtMemoryWidget::ezQtMemoryWidget(QWidget* parent)
  : ads::CDockWidget("Memory Widget", parent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(MemoryWidgetFrame);

  {
    ezQtScopedUpdatesDisabled _1(ComboTimeframe);

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
  }

  m_pPathMax = m_Scene.addPath(QPainterPath(), QPen(QBrush(QColor(255, 255, 255)), 0));

  for (ezUInt32 i = 0; i < s_uiMaxColors; ++i)
    m_pPath[i] = m_Scene.addPath(QPainterPath(), QPen(QBrush(MemoryWidgetDetail::s_Colors[i]), 0));

  QTransform t = UsedMemoryView->transform();
  t.scale(1, -1);
  UsedMemoryView->setTransform(t);

  UsedMemoryView->setScene(&m_Scene);

  UsedMemoryView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  UsedMemoryView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  UsedMemoryView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  // UsedMemoryView->setMaximumHeight(100);

  ListAllocators->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ListAllocators, &QTreeView::customContextMenuRequested, this, &ezQtMemoryWidget::CustomContextMenuRequested);

  ResetStats();
}

void ezQtMemoryWidget::ResetStats()
{
  m_AllocatorData.Clear();

  m_uiMaxSamples = 3000;
  m_uiDisplaySamples = 5 * 60; // 5 samples per second, 60 seconds
  m_uiColorsUsed = 1;
  m_bAllocatorsChanged = true;

  m_Accu = AllocatorData();

  ListAllocators->clear();
}

void ezQtMemoryWidget::UpdateStats()
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

    ezQtScopedUpdatesDisabled _1(ListAllocators);
    ezQtScopedBlockSignals _2(ListAllocators);

    ListAllocators->clear();

    {
      m_Accu.m_pTreeItem = new QTreeWidgetItem();
      m_Accu.m_pTreeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
      m_Accu.m_pTreeItem->setCheckState(0, m_Accu.m_bDisplay ? Qt::Checked : Qt::Unchecked);
      m_Accu.m_pTreeItem->setData(0, Qt::UserRole, ezUInt32(ezInvalidIndex));

      m_Accu.m_pTreeItem->setText(0, "<Accumulated>");
      m_Accu.m_pTreeItem->setForeground(0, QColor(255, 255, 255));

      ListAllocators->addTopLevelItem(m_Accu.m_pTreeItem);
    }

    for (auto it = m_AllocatorData.GetIterator(); it.IsValid(); ++it)
    {
      auto pParentData = m_AllocatorData.Find(it.Value().m_uiParentId);
      QTreeWidgetItem* pParent = pParentData.IsValid() ? pParentData.Value().m_pTreeItem : m_Accu.m_pTreeItem;

      QTreeWidgetItem* pItem = new QTreeWidgetItem(pParent);
      pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
      pItem->setCheckState(0, it.Value().m_bDisplay ? Qt::Checked : Qt::Unchecked);
      pItem->setData(0, Qt::UserRole, it.Key());

      pItem->setText(0, it.Value().m_sName.GetData());
      pItem->setForeground(0, MemoryWidgetDetail::s_Colors[it.Value().m_iColor % s_uiMaxColors]);

      it.Value().m_pTreeItem = pItem;
    }

    ListAllocators->expandAll();
  }

  // once a second update the display of the allocators in the list
  if (ezTime::Now() - m_LastUpdatedAllocatorList > ezTime::Seconds(1))
  {
    m_LastUpdatedAllocatorList = ezTime::Now();

    for (auto it = m_AllocatorData.GetIterator(); it.IsValid(); ++it)
    {
      if (!it.Value().m_pTreeItem || it.Value().m_UsedMemory.IsEmpty())
        continue;

      ezStringBuilder sSize;
      FormatSize(sSize, "", it.Value().m_UsedMemory.PeekBack());

      ezStringBuilder sMaxSize;
      FormatSize(sMaxSize, "", it.Value().m_uiMaxUsedMemory);

      ezStringBuilder sText = it.Value().m_sName;
      sText.AppendFormat(" [{0}]", sSize);

      ezStringBuilder sTooltip;
      sTooltip.Format("<p>Current Memory Used: <b>{0}</b><br>Max Memory Used: <b>{1}</b><br>Live Allocations: <b>{2}</b><br>Allocations: "
                      "<b>{3}</b><br>Deallocations: <b>{4}</b><br>",
        sSize.GetData(), sMaxSize.GetData(), it.Value().m_uiLiveAllocs, it.Value().m_uiAllocs, it.Value().m_uiDeallocs);

      it.Value().m_pTreeItem->setText(0, sText.GetData());
      it.Value().m_pTreeItem->setToolTip(0, sTooltip.GetData());
    }

    if (m_Accu.m_pTreeItem && !m_Accu.m_UsedMemory.IsEmpty())
    {
      ezStringBuilder sSize;
      FormatSize(sSize, "", m_Accu.m_UsedMemory.PeekBack());

      ezStringBuilder sMaxSize;
      FormatSize(sMaxSize, "", m_Accu.m_uiMaxUsedMemory);

      ezStringBuilder sText = "<Accumulated>";
      sText.AppendFormat(" [{0}]", sSize);

      ezStringBuilder sTooltip;
      sTooltip.Format("<p>Current Memory Used: <b>{0}</b><br>Max Memory Used: <b>{1}</b><br>Live Allocations: <b>{2}</b><br>Allocations: "
                      "<b>{3}</b><br>Deallocations: <b>{4}</b><br>",
        sSize.GetData(), sMaxSize.GetData(), m_Accu.m_uiLiveAllocs, m_Accu.m_uiAllocs, m_Accu.m_uiDeallocs);

      m_Accu.m_pTreeItem->setText(0, sText.GetData());
      m_Accu.m_pTreeItem->setToolTip(0, sTooltip.GetData());
    }
  }

  if (ezTime::Now() - s_pWidget->m_LastUsedMemoryStored > ezTime::Milliseconds(200))
  {
    m_LastUsedMemoryStored = ezTime::Now();

    ezUInt64 uiSumMemory = 0;

    for (auto it = m_AllocatorData.GetIterator(); it.IsValid(); ++it)
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

  for (auto it = s_pWidget->m_AllocatorData.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_UsedMemory.IsEmpty() || !it.Value().m_bDisplay)
      continue;

    ezUInt64 uiMinUsedMemoryThis = 0xFFFFFFFF;
    ezUInt64 uiMaxUsedMemoryThis = 0;

    const ezUInt32 uiColorPath = it.Value().m_iColor % s_uiMaxColors;

    uiUsedMemory += it.Value().m_UsedMemory.PeekBack();
    uiLiveAllocs += it.Value().m_uiLiveAllocs;
    uiAllocs += it.Value().m_uiAllocs;
    uiDeallocs += it.Value().m_uiDeallocs;

    if (it.Value().m_UsedMemory.GetCount() > m_uiMaxSamples)
      it.Value().m_UsedMemory.PopFront(it.Value().m_UsedMemory.GetCount() - m_uiMaxSamples);

    uiMaxSamples = ezMath::Max(uiMaxSamples, ezMath::Min(m_uiDisplaySamples, it.Value().m_UsedMemory.GetCount()));

    const ezUInt32 uiFirstSample =
      (it.Value().m_UsedMemory.GetCount() <= m_uiDisplaySamples) ? 0 : (it.Value().m_UsedMemory.GetCount() - m_uiDisplaySamples);
    const ezUInt32 uiStartPos =
      (it.Value().m_UsedMemory.GetCount() >= m_uiDisplaySamples) ? 0 : (m_uiDisplaySamples - it.Value().m_UsedMemory.GetCount());

    pp[uiColorPath].moveTo(QPointF(uiStartPos, it.Value().m_UsedMemory[uiFirstSample]));
    uiMinUsedMemoryThis = ezMath::Min(uiMinUsedMemoryThis, it.Value().m_UsedMemory[uiFirstSample]);
    uiMaxUsedMemoryThis = ezMath::Max(uiMaxUsedMemoryThis, it.Value().m_UsedMemory[uiFirstSample]);

    if (it.Value().m_uiParentId == ezInvalidIndex) // only accumulate top level allocators
    {
      m_Accu.m_uiAllocs += it.Value().m_uiAllocs;
      m_Accu.m_uiDeallocs += it.Value().m_uiDeallocs;
      m_Accu.m_uiLiveAllocs += it.Value().m_uiLiveAllocs;

      m_Accu.m_UsedMemory[uiStartPos] += it.Value().m_UsedMemory[uiFirstSample];
      m_Accu.m_uiMaxUsedMemory += it.Value().m_uiMaxUsedMemory;
    }

    for (ezUInt32 i = uiFirstSample + 1; i < it.Value().m_UsedMemory.GetCount(); ++i)
    {
      pp[uiColorPath].lineTo(QPointF(uiStartPos + i - uiFirstSample, it.Value().m_UsedMemory[i]));

      uiMinUsedMemoryThis = ezMath::Min(uiMinUsedMemoryThis, it.Value().m_UsedMemory[i]);
      uiMaxUsedMemoryThis = ezMath::Max(uiMaxUsedMemoryThis, it.Value().m_UsedMemory[i]);

      if (it.Value().m_uiParentId == ezInvalidIndex)
      {
        m_Accu.m_UsedMemory[uiStartPos + i - uiFirstSample] += it.Value().m_UsedMemory[i];
      }

      if (m_Accu.m_bDisplay)
        uiMaxUsedMemoryThis = ezMath::Max(uiMaxUsedMemoryThis, m_Accu.m_UsedMemory[uiStartPos + i - uiFirstSample]);
    }

    uiMinUsedMemory = ezMath::Min(uiMinUsedMemory, uiMinUsedMemoryThis);
    uiMaxUsedMemory = ezMath::Max(uiMaxUsedMemory, uiMaxUsedMemoryThis);
  }

  QPainterPath pMax;

  if (m_Accu.m_bDisplay)
  {
    pMax.moveTo(QPointF(0, m_Accu.m_UsedMemory[0]));

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
    UsedMemoryView->setSceneRect(QRectF(0, uiMinUsedMemory, m_uiDisplaySamples - 1, uiMaxUsedMemory));
    UsedMemoryView->fitInView(QRectF(0, uiMinUsedMemory, m_uiDisplaySamples - 1, uiMaxUsedMemory));

    ezStringBuilder s;

    FormatSize(s, "Min: ", uiMinUsedMemory);
    LabelMinMemory->setText(QString::fromUtf8(s.GetData()));

    s.Format("<p>Recent Minimum Memory Usage:<br>{0} GB<br>{1} MB<br>{2} KB<br>{3} Byte</p>",
      ezArgF(uiMinUsedMemory / 1024.0 / 1024.0 / 1024.0, 2), ezArgF(uiMinUsedMemory / 1024.0 / 1024.0, 2),
      ezArgF(uiMinUsedMemory / 1024.0, 2), uiMinUsedMemory);
    LabelMinMemory->setToolTip(QString::fromUtf8(s.GetData()));

    FormatSize(s, "Max: ", uiMaxUsedMemory);
    LabelMaxMemory->setText(QString::fromUtf8(s.GetData()));

    s.Format("<p>Recent Maximum Memory Usage:<br>{0} GB<br>{1} MB<br>{2} KB<br>{3} Byte</p>",
      ezArgF(uiMaxUsedMemory / 1024.0 / 1024.0 / 1024.0, 2), ezArgF(uiMaxUsedMemory / 1024.0 / 1024.0, 2),
      ezArgF(uiMaxUsedMemory / 1024.0, 2), uiMaxUsedMemory);
    LabelMaxMemory->setToolTip(QString::fromUtf8(s.GetData()));

    const ezUInt64 uiCurUsedMemory = uiUsedMemory;

    FormatSize(s, "Sum: ", uiCurUsedMemory);
    LabelCurMemory->setText(QString::fromUtf8(s.GetData()));

    s.Format("<p>Current Memory Usage:<br>{0} GB<br>{1} MB<br>{2} KB<br>{3} Byte</p>",
      ezArgF(uiCurUsedMemory / 1024.0 / 1024.0 / 1024.0, 2), ezArgF(uiCurUsedMemory / 1024.0 / 1024.0, 2),
      ezArgF(uiCurUsedMemory / 1024.0, 2), uiCurUsedMemory);
    LabelCurMemory->setToolTip(QString::fromUtf8(s.GetData()));

    s.Format("Allocs: {0}", uiLiveAllocs);
    LabelNumLiveAllocs->setText(QString::fromUtf8(s.GetData()));

    s.Format("Counter: {0} / {1}", uiAllocs, uiDeallocs);
    LabelNumAllocs->setText(QString::fromUtf8(s.GetData()));

    s.Format("<p>Allocations: <b>{0}</b><br>Deallocations: <b>{1}</b></p>", uiAllocs, uiDeallocs);
    LabelNumAllocs->setToolTip(QString::fromUtf8(s.GetData()));
  }
}

void ezQtMemoryWidget::CustomContextMenuRequested(const QPoint& pos)
{
  QModelIndex CurrentIndex = ListAllocators->currentIndex();

  QMenu m;
  if (CurrentIndex.isValid())
  {
    m.addAction(actionEnableOnlyThis);
    m.addSeparator();
  }

  m.addAction(actionEnableAll);
  m.addAction(actionDisableAll);

  m.exec(ListAllocators->viewport()->mapToGlobal(pos));
}

void ezQtMemoryWidget::ProcessTelemetry(void* pUnuseed)
{
  if (s_pWidget == nullptr)
    return;

  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage(' MEM', Msg) == EZ_SUCCESS)
  {
    if (Msg.GetMessageID() == 'BGN')
    {
      for (auto it : s_pWidget->m_AllocatorData)
      {
        it.Value().m_bStillInUse = false;
      }
    }

    if (Msg.GetMessageID() == 'END')
    {
      for (auto it = s_pWidget->m_AllocatorData.GetIterator(); it.IsValid();)
      {
        if (!it.Value().m_bStillInUse)
        {
          s_pWidget->m_bAllocatorsChanged = true;
          it = s_pWidget->m_AllocatorData.Remove(it);
        }
        else
        {
          ++it;
        }
      }
    }

    if (Msg.GetMessageID() == 'STAT')
    {
      ezString sAllocatorName;
      ezUInt32 uiAllocatorId;
      ezUInt32 uiParentId;

      ezAllocatorBase::Stats MemStat;
      Msg.GetReader() >> uiAllocatorId;
      Msg.GetReader() >> sAllocatorName;
      Msg.GetReader() >> uiParentId;
      Msg.GetReader() >> MemStat;

      AllocatorData& ad = s_pWidget->m_AllocatorData[uiAllocatorId];
      ad.m_bStillInUse = true;
      ad.m_bReceivedData = true;
      ad.m_sName = sAllocatorName;
      ad.m_uiParentId = uiParentId;

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
}

void ezQtMemoryWidget::on_ListAllocators_itemChanged(QTreeWidgetItem* item)
{
  if (item->data(0, Qt::UserRole).toUInt() == ezInvalidIndex)
  {
    m_Accu.m_bDisplay = (item->checkState(0) == Qt::Checked);
    return;
  }

  m_AllocatorData[item->data(0, Qt::UserRole).toUInt()].m_bDisplay = (item->checkState(0) == Qt::Checked);
}

void ezQtMemoryWidget::on_ComboTimeframe_currentIndexChanged(int index)
{
  m_uiDisplaySamples = 5 * 60 * (index + 1); // 5 samples per second, 60 seconds
}

void ezQtMemoryWidget::on_actionEnableOnlyThis_triggered(bool)
{
  on_actionDisableAll_triggered(false);

  m_AllocatorData[ListAllocators->currentItem()->data(0, Qt::UserRole).toUInt()].m_bDisplay = true;
}

void ezQtMemoryWidget::on_actionEnableAll_triggered(bool)
{
  m_bAllocatorsChanged = true;

  for (auto it : m_AllocatorData)
  {
    it.Value().m_bDisplay = true;
  }
}

void ezQtMemoryWidget::on_actionDisableAll_triggered(bool)
{
  m_bAllocatorsChanged = true;

  for (auto it : m_AllocatorData)
  {
    it.Value().m_bDisplay = false;
  }
}

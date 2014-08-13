#include <PCH.h>
#include <Inspector/TimeWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <QGraphicsView>
#include <QGraphicsPathItem>

ezTimeWidget* ezTimeWidget::s_pWidget = nullptr;

static QColor s_Colors[ezTimeWidget::s_uiMaxColors] =
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

ezTimeWidget::ezTimeWidget(QWidget* parent) : QDockWidget (parent)
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

  m_pPathMax = m_Scene.addPath (QPainterPath (), QPen (QBrush (QColor(64, 64, 64)), 0 ));

  for (ezUInt32 i = 0; i < s_uiMaxColors; ++i)
    m_pPath[i] = m_Scene.addPath (QPainterPath (), QPen (QBrush (s_Colors[i]), 0 ));

  QTransform t = TimeView->transform ();
  t.scale (1, -1);
  TimeView->setTransform (t);

  TimeView->setScene(&m_Scene);

  TimeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  TimeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  TimeView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  //TimeView->setMaximumHeight(100);

  ResetStats();
}

void ezTimeWidget::ResetStats()
{
  m_ClockData.Clear();

  m_uiMaxSamples = 40000;
  m_DisplayInterval = ezTime::Seconds(60.0);
  m_uiColorsUsed = 1;
  m_bClocksChanged = true;

  ListClocks->clear();
}

void ezTimeWidget::UpdateStats()
{
  if (!isVisible())
    return;

  if (!ezTelemetry::IsConnectedToServer())
  {
    ListClocks->setEnabled(false);
    return;
  }

  ListClocks->setEnabled(true);

  if (m_bClocksChanged)
  {
    m_bClocksChanged = false;

    ListClocks->blockSignals(true);
    ListClocks->clear();

    for (ezMap<ezString, ezTimeWidget::ClockData>::Iterator it = m_ClockData.GetIterator(); it.IsValid(); ++it)
    {
      ListClocks->addItem(it.Key().GetData());

      QListWidgetItem* pItem = ListClocks->item(ListClocks->count() - 1);
      pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
      pItem->setCheckState (it.Value().m_bDisplay ? Qt::Checked : Qt::Unchecked);
      pItem->setData(Qt::UserRole, QString(it.Key().GetData()));

      pItem->setTextColor(s_Colors[it.Value().m_iColor % s_uiMaxColors]);

      it.Value().m_pListItem = pItem;
    }

    ListClocks->blockSignals(false);
  }

  QPainterPath pp[s_uiMaxColors];

  ezTime tMin = ezTime::Seconds(100.0);
  ezTime tMax = ezTime::Seconds(0.0);

  for (ezMap<ezString, ClockData>::Iterator it = s_pWidget->m_ClockData.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_TimeSamples.IsEmpty() || !it.Value().m_bDisplay)
      continue;

    const ezUInt32 uiColorPath = it.Value().m_iColor % s_uiMaxColors;
    ClockData& Clock = it.Value();
    const ezDeque<TimeSample>& Samples = Clock.m_TimeSamples;

    ezUInt32 uiFirstSample = 0;

    while ((uiFirstSample < Samples.GetCount()) && (m_MaxGlobalTime - Samples[uiFirstSample].m_AtGlobalTime > m_DisplayInterval))
      ++uiFirstSample;

    if (uiFirstSample < Samples.GetCount())
    {
      pp[uiColorPath].moveTo (QPointF((Samples[uiFirstSample].m_AtGlobalTime - m_MaxGlobalTime).GetSeconds(), Samples[uiFirstSample].m_Timestep.GetSeconds()));

      for (ezUInt32 i = uiFirstSample + 1; i < Samples.GetCount(); ++i)
      {
        pp[uiColorPath].lineTo (QPointF ((Samples[i].m_AtGlobalTime - m_MaxGlobalTime).GetSeconds(), Samples[i].m_Timestep.GetSeconds()));

        tMin = ezMath::Min(tMin, Samples[i].m_Timestep);
        tMax = ezMath::Max(tMax, Samples[i].m_Timestep);
      }
    }
  }

  for (ezUInt32 i = 0; i < s_uiMaxColors; ++i)
    m_pPath[i]->setPath(pp[i]);

  // render the helper lines for time values
  {
    QPainterPath pMax;

    for (ezUInt32 i = 1; i < 10; ++i)
    {
      pMax.moveTo(QPointF (-m_DisplayInterval.GetSeconds(), ezTime::Milliseconds(10.0 * i).GetSeconds()));
      pMax.lineTo(QPointF (0, ezTime::Milliseconds(10.0 * i).GetSeconds()));
    }

    m_pPathMax->setPath(pMax);
  }

  ezTime tShowMax = ezTime::Seconds(1.0 / 10.0);

  for (ezUInt32 t = 25; t < 100; t += 25)
  {
    tShowMax = ezTime::Milliseconds(1) * t;

    if (tMax < tShowMax)
      break;
  }

  {
    TimeView->setSceneRect (QRectF (-m_DisplayInterval.GetSeconds(), 0, m_DisplayInterval.GetSeconds(), tShowMax.GetSeconds()));
    TimeView->fitInView    (QRectF (-m_DisplayInterval.GetSeconds(), 0, m_DisplayInterval.GetSeconds(), tShowMax.GetSeconds()));
  }

  // once a second update the display of the clocks in the list
  if (ezTime::Now() - m_LastUpdatedClockList > ezTime::Seconds(1))
  {
    m_LastUpdatedClockList = ezTime::Now();

    ezStringBuilder s;
    s.Format("Max: %.0fms", tShowMax.GetMilliseconds());
    LabelMaxTime->setText(s.GetData());

    for (ezMap<ezString, ezTimeWidget::ClockData>::Iterator it = m_ClockData.GetIterator(); it.IsValid(); ++it)
    {
      const ezTimeWidget::ClockData& Clock = it.Value();

      if (!Clock.m_pListItem || Clock.m_TimeSamples.IsEmpty())
        continue;

      ezStringBuilder sTooltip;
      sTooltip.Format("<p>Clock: %s<br>Max Time Step: <b>%.2fms</b><br>Min Time Step: <b>%.2fms</b><br></p>",
        it.Key().GetData(), Clock.m_MaxTimestep.GetMilliseconds(), Clock.m_MinTimestep.GetMilliseconds());

      Clock.m_pListItem->setToolTip(sTooltip.GetData());
    }
  }
}

void ezTimeWidget::ProcessTelemetry(void* pUnuseed)
{
  if (s_pWidget == nullptr)
    return;

  ezTelemetryMessage Msg;

  ezStringBuilder sTemp;

  while (ezTelemetry::RetrieveMessage('TIME', Msg) == EZ_SUCCESS)
  {
    ezString sClockName;
    Msg.GetReader() >> sClockName;

    sTemp.Format("%s [smoothed]", sClockName.GetData());

    ClockData& ad = s_pWidget->m_ClockData[sClockName];
    ClockData& ads = s_pWidget->m_ClockData[sTemp.GetData()];

    TimeSample Sample;
    TimeSample SampleSmooth;

    Msg.GetReader() >> Sample.m_AtGlobalTime;
    Msg.GetReader() >> Sample.m_Timestep;

    SampleSmooth.m_AtGlobalTime = Sample.m_AtGlobalTime;
    Msg.GetReader() >> SampleSmooth.m_Timestep;

    s_pWidget->m_MaxGlobalTime = ezMath::Max(s_pWidget->m_MaxGlobalTime, Sample.m_AtGlobalTime);

    if (ad.m_TimeSamples.GetCount() > 1 && (ezMath::IsEqual(ad.m_TimeSamples.PeekBack().m_Timestep, Sample.m_Timestep, ezTime::Microseconds(100))))
      ad.m_TimeSamples.PeekBack() = Sample;
    else
      ad.m_TimeSamples.PushBack(Sample);

    if (ads.m_TimeSamples.GetCount() > 1 && (ezMath::IsEqual(ads.m_TimeSamples.PeekBack().m_Timestep, SampleSmooth.m_Timestep, ezTime::Microseconds(100))))
      ads.m_TimeSamples.PeekBack() = SampleSmooth;
    else
      ads.m_TimeSamples.PushBack(SampleSmooth);

    ad.m_MinTimestep = ezMath::Min(ad.m_MinTimestep, Sample.m_Timestep);
    ad.m_MaxTimestep = ezMath::Max(ad.m_MaxTimestep, Sample.m_Timestep);

    ads.m_MinTimestep = ezMath::Min(ads.m_MinTimestep, SampleSmooth.m_Timestep);
    ads.m_MaxTimestep = ezMath::Max(ads.m_MaxTimestep, SampleSmooth.m_Timestep);

    if (ad.m_iColor < 0)
    {
      ad.m_iColor = s_pWidget->m_uiColorsUsed;
      ++s_pWidget->m_uiColorsUsed;
      s_pWidget->m_bClocksChanged = true;
    }

    if (ads.m_iColor < 0)
    {
      ads.m_iColor = s_pWidget->m_uiColorsUsed;
      ++s_pWidget->m_uiColorsUsed;
      s_pWidget->m_bClocksChanged = true;
    }

    if (ad.m_TimeSamples.GetCount() > s_pWidget->m_uiMaxSamples)
      ad.m_TimeSamples.PopFront(ad.m_TimeSamples.GetCount() - s_pWidget->m_uiMaxSamples);

    if (ads.m_TimeSamples.GetCount() > s_pWidget->m_uiMaxSamples)
      ads.m_TimeSamples.PopFront(ads.m_TimeSamples.GetCount() - s_pWidget->m_uiMaxSamples);
  }
}

void ezTimeWidget::on_ListClocks_itemChanged(QListWidgetItem* item)
{
  m_ClockData[item->data(Qt::UserRole).toString().toUtf8().data()].m_bDisplay = (item->checkState() == Qt::Checked);
}

void ezTimeWidget::on_ComboTimeframe_currentIndexChanged(int index)
{
  m_DisplayInterval = ezTime::Seconds(60.0) * (index + 1);
}
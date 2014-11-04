#include <PCH.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/StatVisWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <QGraphicsView>
#include <QGraphicsPathItem>
#include <QSettings>

ezStatVisWidget* ezStatVisWidget::s_pWidget = nullptr;
ezInt32 ezStatVisWidget::s_iCurColor = 0;

static QColor s_Colors[ezStatVisWidget::s_uiMaxColors] =
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

ezStatVisWidget::ezStatVisWidget(QWidget* parent, ezInt32 iWindowNumber) : QDockWidget (parent), m_ShowWindowAction(parent)
{
  m_iWindowNumber = iWindowNumber;
  m_DisplayInterval = ezTime::Seconds(60.0);

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

  ButtonRemove->setEnabled(false);

  m_pPathMax = m_Scene.addPath (QPainterPath (), QPen (QBrush (QColor(64, 64, 64)), 0 ));

  for (ezUInt32 i = 0; i < s_uiMaxColors; ++i)
    m_pPath[i] = m_Scene.addPath (QPainterPath (), QPen (QBrush (s_Colors[i]), 0 ));

  QTransform t = StatHistoryView->transform ();
  t.scale (1, -1);
  StatHistoryView->setTransform (t);

  StatHistoryView->setScene(&m_Scene);

  StatHistoryView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  StatHistoryView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  StatHistoryView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  //TimeView->setMaximumHeight(100);

  m_ShowWindowAction.setCheckable(true);

  ezStringBuilder sStatHistory;
  sStatHistory.Format("StatHistory%i", m_iWindowNumber);

  QSettings Settings;
  Settings.beginGroup(sStatHistory.GetData());
  LineName->setText(Settings.value(QLatin1String("Name"), QLatin1String(sStatHistory.GetData())).toString());
  SpinMin->setValue(Settings.value(QLatin1String("Min"), 0.0).toDouble());
  SpinMax->setValue(Settings.value(QLatin1String("Max"), 1.0).toDouble());
  Settings.endGroup();

  EZ_VERIFY(nullptr != QWidget::connect(&m_ShowWindowAction,  SIGNAL(triggered()), this, SLOT(on_ToggleVisible())), "");

  
}

void ezStatVisWidget::on_ComboTimeframe_currentIndexChanged(int index)
{
  m_DisplayInterval = ezTime::Seconds(60.0) * (index + 1);
}

void ezStatVisWidget::on_LineName_textChanged(const QString& text)
{
  ezStringBuilder sStatHistory;
  sStatHistory.Format("StatHistory%i", m_iWindowNumber);

  QSettings Settings;
  Settings.beginGroup(sStatHistory.GetData());
  Settings.setValue(QLatin1String("Name"), LineName->text());
  Settings.endGroup();

  setWindowTitle(QLatin1String("Stats: ") + LineName->text());
  m_ShowWindowAction.setText(LineName->text());
}

void ezStatVisWidget::on_SpinMin_valueChanged(double val)
{
  ezStringBuilder sStatHistory;
  sStatHistory.Format("StatHistory%i", m_iWindowNumber);

  QSettings Settings;
  Settings.beginGroup(sStatHistory.GetData());
  Settings.setValue(QLatin1String("Min"), val);
  Settings.endGroup();
}

void ezStatVisWidget::on_SpinMax_valueChanged(double val)
{
  ezStringBuilder sStatHistory;
  sStatHistory.Format("StatHistory%i", m_iWindowNumber);

  QSettings Settings;
  Settings.beginGroup(sStatHistory.GetData());
  Settings.setValue(QLatin1String("Max"), val);
  Settings.endGroup();
}

void ezStatVisWidget::on_ToggleVisible()
{
  setVisible(!isVisible());

  if (isVisible())
    raise();
}

void ezStatVisWidget::on_ListStats_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
  ButtonRemove->setEnabled(current != nullptr);
}

void ezStatVisWidget::on_ButtonRemove_clicked()
{
  if (ListStats->currentItem() == nullptr)
    return;

  for (auto it = m_Stats.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_pListItem == ListStats->currentItem())
    {
      delete it.Value().m_pListItem;
      m_Stats.Remove(it);
      break;
    }
  }
}

void ezStatVisWidget::Save()
{
  ezStringBuilder sStatHistory;
  sStatHistory.Format("/StatWindow%i.stats", m_iWindowNumber);

  QString sFile = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  QDir d; d.mkpath(sFile);

  sFile.append(sStatHistory.GetData());

  QFile f(sFile);
  if (!f.open(QIODevice::WriteOnly))
    return;

  QDataStream data(&f);

  const ezUInt32 uiNumFavourites = m_Stats.GetCount();
  data << uiNumFavourites;

  for (auto it = m_Stats.GetIterator(); it.IsValid(); ++it)
  {
    const QString s = it.Key().GetData();
    data << s;
    data << (it.Value().m_pListItem->checkState() == Qt::Checked);
  }

  f.close();
}

void ezStatVisWidget::Load()
{
  ezStringBuilder sStatHistory;
  sStatHistory.Format("/StatWindow%i.stats", m_iWindowNumber);

  QString sFile = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  QDir d; d.mkpath(sFile);

  sFile.append(sStatHistory.GetData());

  QFile f(sFile);
  if (!f.open(QIODevice::ReadOnly))
    return;

  m_Stats.Clear();

  QDataStream data(&f);

  ezUInt32 uiNumFavourites = 0;
  data >> uiNumFavourites;

  for (ezUInt32 i = 0; i < uiNumFavourites; ++i)
  {
    QString s;
    data >> s;

    bool bChecked = true;
    data >> bChecked;

    ezString ezs = s.toUtf8().data();

    AddStat(ezs, bChecked, false);
  }

  f.close();
}

void ezStatVisWidget::AddStat(const ezString& sStatPath, bool bEnabled, bool bRaiseWindow)
{
  if (bRaiseWindow)
  {
    setVisible(true);
    raise();
  }

  StatsData& Stat = m_Stats[sStatPath];

  if (Stat.m_pListItem == nullptr)
  {
    Stat.m_uiColor = s_iCurColor % ezStatVisWidget::s_uiMaxColors;
    ++s_iCurColor;

    Stat.m_pListItem = new QListWidgetItem();
    Stat.m_pListItem->setText(sStatPath.GetData());
    Stat.m_pListItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
    Stat.m_pListItem->setCheckState(bEnabled ? Qt::Checked : Qt::Unchecked);
    Stat.m_pListItem->setTextColor(s_Colors[Stat.m_uiColor]);

    ListStats->addItem(Stat.m_pListItem);
  }
}

void ezStatVisWidget::UpdateStats()
{
  if (!isVisible())
    return;

  QPainterPath pp[s_uiMaxColors];

  ezTime tMin = ezTime::Seconds(100.0);
  ezTime tMax = ezTime::Seconds(0.0);

  for (ezMap<ezString, StatsData>::Iterator it = m_Stats.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_pListItem->checkState() != Qt::Checked)
      continue;

    const ezDeque<ezMainWindow::StatSample>& Samples = ezMainWindow::s_pWidget->m_Stats[it.Key()].m_History;

    if (Samples.IsEmpty())
      continue;

    const ezUInt32 uiColorPath = it.Value().m_uiColor;

    ezUInt32 uiFirstSample = 0;

    const ezTime MaxGlobalTime = ezMainWindow::s_pWidget->m_MaxGlobalTime;

    while ((uiFirstSample + 1 < Samples.GetCount()) && (MaxGlobalTime - Samples[uiFirstSample + 1].m_AtGlobalTime > m_DisplayInterval))
      ++uiFirstSample;

    if (uiFirstSample < Samples.GetCount())
    {
      pp[uiColorPath].moveTo(QPointF((Samples[uiFirstSample].m_AtGlobalTime - MaxGlobalTime).GetSeconds(), Samples[uiFirstSample].m_Value));

      for (ezUInt32 i = uiFirstSample + 1; i < Samples.GetCount(); ++i)
      {
        pp[uiColorPath].lineTo(QPointF((Samples[i].m_AtGlobalTime - MaxGlobalTime).GetSeconds(), Samples[i].m_Value));
      }

      const double dLatestValue = Samples.PeekBack().m_Value;
      pp[uiColorPath].lineTo(QPointF(0.0, dLatestValue));
    }
  }

  for (ezUInt32 i = 0; i < s_uiMaxColors; ++i)
    m_pPath[i]->setPath(pp[i]);

  {
    double dMin = SpinMin->value();
    double dHeight = SpinMax->value() - SpinMin->value();

    StatHistoryView->setSceneRect (QRectF (-m_DisplayInterval.GetSeconds(), dMin, m_DisplayInterval.GetSeconds(), dHeight));
    StatHistoryView->fitInView    (QRectF (-m_DisplayInterval.GetSeconds(), dMin, m_DisplayInterval.GetSeconds(), dHeight));
  }
}




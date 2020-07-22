#include <InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <Inspector/MainWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/StatVisWidget.moc.h>
#include <QGraphicsPathItem>
#include <QGraphicsView>
#include <QSettings>

ezQtStatVisWidget* ezQtStatVisWidget::s_pWidget = nullptr;
ezInt32 ezQtStatVisWidget::s_iCurColor = 0;

namespace StatVisWidgetDetail
{
  static QColor s_Colors[ezQtStatVisWidget::s_uiMaxColors] = {
    QColor(255, 106, 0), // orange
    QColor(182, 255, 0), // lime green
    QColor(255, 0, 255), // pink
    QColor(0, 148, 255), // light blue
    QColor(255, 0, 0),   // red
    QColor(0, 255, 255), // turquoise
    QColor(178, 0, 255), // purple
    QColor(0, 38, 255),  // dark blue
    QColor(72, 0, 255),  // lilac
  };
}

ezQtStatVisWidget::ezQtStatVisWidget(QWidget* parent, ezInt32 iWindowNumber)
  : ads::CDockWidget(QString("StatVisWidget") + QString::number(iWindowNumber), parent)
  , m_ShowWindowAction(parent)
{
  m_iWindowNumber = iWindowNumber;
  m_DisplayInterval = ezTime::Seconds(60.0);

  s_pWidget = this;

  setupUi(this);
  setWidget(StatVisWidgetFrame);

  {
    ezQtScopedUpdatesDisabled _1(ComboTimeframe);

    ComboTimeframe->addItem("Timeframe: 10 seconds");
    ComboTimeframe->addItem("Timeframe: 30 seconds");
    ComboTimeframe->addItem("Timeframe: 1 minute");
    ComboTimeframe->addItem("Timeframe: 2 minutes");
    ComboTimeframe->addItem("Timeframe: 5 minutes");
    ComboTimeframe->addItem("Timeframe: 10 minutes");
    ComboTimeframe->setCurrentIndex(2);
  }

  ButtonRemove->setEnabled(false);

  m_pPathMax = m_Scene.addPath(QPainterPath(), QPen(QBrush(QColor(64, 64, 64)), 0));

  for (ezUInt32 i = 0; i < s_uiMaxColors; ++i)
    m_pPath[i] = m_Scene.addPath(QPainterPath(), QPen(QBrush(StatVisWidgetDetail::s_Colors[i]), 0));

  QTransform t = StatHistoryView->transform();
  t.scale(1, -1);
  StatHistoryView->setTransform(t);

  StatHistoryView->setScene(&m_Scene);

  StatHistoryView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  StatHistoryView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  StatHistoryView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  // TimeView->setMaximumHeight(100);

  m_ShowWindowAction.setCheckable(true);

  ezStringBuilder sStatHistory;
  sStatHistory.Format("StatHistory{0}", m_iWindowNumber);

  QSettings Settings;
  Settings.beginGroup(sStatHistory.GetData());
  LineName->setText(Settings.value(QLatin1String("Name"), QLatin1String(sStatHistory.GetData())).toString());
  SpinMin->setValue(Settings.value(QLatin1String("Min"), 0.0).toDouble());
  SpinMax->setValue(Settings.value(QLatin1String("Max"), 1.0).toDouble());
  Settings.endGroup();

  EZ_VERIFY(nullptr != QWidget::connect(&m_ShowWindowAction, SIGNAL(triggered()), this, SLOT(on_ToggleVisible())), "");
}


ezQtStatVisWidget::~ezQtStatVisWidget() {}

void ezQtStatVisWidget::on_ComboTimeframe_currentIndexChanged(int index)
{
  const ezUInt32 uiSeconds[] = {
    10,
    30,
    60 * 1,
    60 * 2,
    60 * 5,
    60 * 10,
  };

  m_DisplayInterval = ezTime::Seconds(uiSeconds[index]);
}

void ezQtStatVisWidget::on_LineName_textChanged(const QString& text)
{
  ezStringBuilder sStatHistory;
  sStatHistory.Format("StatHistory{0}", m_iWindowNumber);

  QSettings Settings;
  Settings.beginGroup(sStatHistory.GetData());
  Settings.setValue(QLatin1String("Name"), LineName->text());
  Settings.endGroup();

  setWindowTitle(QLatin1String("Stats: ") + LineName->text());
  m_ShowWindowAction.setText(LineName->text());
}

void ezQtStatVisWidget::on_SpinMin_valueChanged(double val)
{
  ezStringBuilder sStatHistory;
  sStatHistory.Format("StatHistory{0}", m_iWindowNumber);

  QSettings Settings;
  Settings.beginGroup(sStatHistory.GetData());
  Settings.setValue(QLatin1String("Min"), val);
  Settings.endGroup();
}

void ezQtStatVisWidget::on_SpinMax_valueChanged(double val)
{
  ezStringBuilder sStatHistory;
  sStatHistory.Format("StatHistory{0}", m_iWindowNumber);

  QSettings Settings;
  Settings.beginGroup(sStatHistory.GetData());
  Settings.setValue(QLatin1String("Max"), val);
  Settings.endGroup();
}

void ezQtStatVisWidget::on_ToggleVisible()
{
  toggleView(isClosed());

  if (!isClosed())
    raise();
}

void ezQtStatVisWidget::on_ListStats_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
  ButtonRemove->setEnabled(current != nullptr);
}

void ezQtStatVisWidget::on_ButtonRemove_clicked()
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

void ezQtStatVisWidget::Save()
{
  ezStringBuilder sStatHistory;
  sStatHistory.Format("/StatWindow{0}.stats", m_iWindowNumber);

  QString sFile = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  QDir dir;
  dir.mkpath(sFile);

  sFile.append(sStatHistory.GetData());

  QFile f(sFile);
  if (!f.open(QIODevice::WriteOnly))
    return;

  QDataStream stream(&f);

  const ezUInt32 uiNumFavorites = m_Stats.GetCount();
  stream << uiNumFavorites;

  for (auto it = m_Stats.GetIterator(); it.IsValid(); ++it)
  {
    const QString s = it.Key().GetData();
    stream << s;
    stream << (it.Value().m_pListItem->checkState() == Qt::Checked);
  }

  f.close();
}

void ezQtStatVisWidget::Load()
{
  ezStringBuilder sStatHistory;
  sStatHistory.Format("/StatWindow{0}.stats", m_iWindowNumber);

  QString sFile = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
  QDir dir;
  dir.mkpath(sFile);

  sFile.append(sStatHistory.GetData());

  QFile f(sFile);
  if (!f.open(QIODevice::ReadOnly))
    return;

  m_Stats.Clear();

  QDataStream stream(&f);

  ezUInt32 uiNumFavorites = 0;
  stream >> uiNumFavorites;

  for (ezUInt32 i = 0; i < uiNumFavorites; ++i)
  {
    QString s;
    stream >> s;

    bool bChecked = true;
    stream >> bChecked;

    ezString ezs = s.toUtf8().data();

    AddStat(ezs, bChecked, false);
  }

  f.close();
}

void ezQtStatVisWidget::AddStat(const ezString& sStatPath, bool bEnabled, bool bRaiseWindow)
{
  if (bRaiseWindow)
  {
    toggleView(true);
    raise();
  }

  StatsData& Stat = m_Stats[sStatPath];

  if (Stat.m_pListItem == nullptr)
  {
    Stat.m_uiColor = s_iCurColor % ezQtStatVisWidget::s_uiMaxColors;
    ++s_iCurColor;

    Stat.m_pListItem = new QListWidgetItem();
    Stat.m_pListItem->setText(sStatPath.GetData());
    Stat.m_pListItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
    Stat.m_pListItem->setCheckState(bEnabled ? Qt::Checked : Qt::Unchecked);
    Stat.m_pListItem->setForeground(StatVisWidgetDetail::s_Colors[Stat.m_uiColor]);

    ListStats->addItem(Stat.m_pListItem);
  }
}

void ezQtStatVisWidget::UpdateStats()
{
  if (isClosed())
    return;

  QPainterPath pp[s_uiMaxColors];

  for (ezMap<ezString, StatsData>::Iterator it = m_Stats.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_pListItem->checkState() != Qt::Checked)
      continue;

    const ezDeque<ezQtMainWidget::StatSample>& Samples = ezQtMainWidget::s_pWidget->m_Stats[it.Key()].m_History;

    if (Samples.IsEmpty())
      continue;

    const ezUInt32 uiColorPath = it.Value().m_uiColor;

    ezUInt32 uiFirstSample = 0;

    const ezTime MaxGlobalTime = ezQtMainWidget::s_pWidget->m_MaxGlobalTime;

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

    StatHistoryView->setSceneRect(QRectF(-m_DisplayInterval.GetSeconds(), dMin, m_DisplayInterval.GetSeconds(), dHeight));
    StatHistoryView->fitInView(QRectF(-m_DisplayInterval.GetSeconds(), dMin, m_DisplayInterval.GetSeconds(), dHeight));
  }
}

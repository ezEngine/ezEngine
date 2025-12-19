#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Panels/AssetCuratorPanel/AssetProcessorProgressWidget.moc.h>
#include <Foundation/Strings/PathUtils.h>
#include <QPainter>
#include <QTimer>
#include <QToolTip>

ezQtAssetProcessorProgressWidget::ezQtAssetProcessorProgressWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setMinimumHeight(200);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  // Connect to asset processor events
  ezAssetProcessor::GetSingleton()->m_ProgressEvents.AddEventHandler(
    ezMakeDelegate(&ezQtAssetProcessorProgressWidget::OnProgressEvent, this));

  // Setup update timer for smooth animation
  m_pUpdateTimer = new QTimer(this);
  connect(m_pUpdateTimer, &QTimer::timeout, this, &ezQtAssetProcessorProgressWidget::OnUpdateTimer);
  m_pUpdateTimer->start(100); // Update 10 times per second
}

ezQtAssetProcessorProgressWidget::~ezQtAssetProcessorProgressWidget()
{
  if (ezAssetProcessor::GetSingleton())
  {
    ezAssetProcessor::GetSingleton()->m_ProgressEvents.RemoveEventHandler(
      ezMakeDelegate(&ezQtAssetProcessorProgressWidget::OnProgressEvent, this));
  }

  if (m_pUpdateTimer)
  {
    m_pUpdateTimer->stop();
  }
}

void ezQtAssetProcessorProgressWidget::SetMaxProcessors(ezUInt32 uiCount)
{
  m_uiMaxProcessors = uiCount;
  m_ProcessorHistory.SetCount(uiCount);

  // Update widget size based on processor count
  int minHeight = s_iTopMargin + (s_iRowHeight + s_iRowSpacing) * uiCount + s_iRowSpacing;
  setMinimumHeight(minHeight);

  update();
}

void ezQtAssetProcessorProgressWidget::OnProgressEvent(const ezAssetProcessorProgressEvent& e)
{
  // Make sure we have space for this processor
  if (e.m_uiProcessorID >= m_ProcessorHistory.GetCount())
  {
    SetMaxProcessors(e.m_uiProcessorID + 1);
  }

  auto& history = m_ProcessorHistory[e.m_uiProcessorID];

  if (e.m_Type == ezAssetProcessorProgressEvent::Type::ProcessingStarted)
  {
    // Add new task
    ProcessorTask task;
    task.m_AssetGuid = e.m_AssetGuid;
    task.m_sAssetPath = e.m_sAssetPath;
    task.m_StartTime = e.m_StartTime;
    task.m_bFinished = false;
    history.PushBack(task);
  }
  else if (e.m_Type == ezAssetProcessorProgressEvent::Type::ProcessingFinished)
  {
    // Find and update the corresponding task
    for (ezInt32 i = history.GetCount() - 1; i >= 0; --i)
    {
      if (history[i].m_AssetGuid == e.m_AssetGuid && !history[i].m_bFinished)
      {
        history[i].m_EndTime = e.m_EndTime;
        history[i].m_bFinished = true;
        history[i].m_bSuccess = e.m_Result.Succeeded();
        break;
      }
    }
  }

  // Clean up old history (keep only last 5 minutes of data)
  ezTime currentTime = ezTime::Now();
  ezTime cleanupThreshold = ezTime::MakeFromSeconds(300.0); // 5 minutes

  for (auto& processorHistory : m_ProcessorHistory)
  {
    while (!processorHistory.IsEmpty())
    {
      const auto& task = processorHistory[0];
      ezTime taskTime = task.m_bFinished ? task.m_EndTime : task.m_StartTime;
      
      if ((currentTime - taskTime) > cleanupThreshold)
      {
        processorHistory.RemoveAtAndCopy(0);
      }
      else
      {
        break;
      }
    }
  }

  update();
}

void ezQtAssetProcessorProgressWidget::OnUpdateTimer()
{
  // Trigger repaint for smooth timeline scrolling
  update();
}

void ezQtAssetProcessorProgressWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Fill background
  painter.fillRect(rect(), QColor(45, 45, 48));

  if (m_uiMaxProcessors == 0)
  {
    painter.setPen(QColor(200, 200, 200));
    painter.drawText(rect(), Qt::AlignCenter, "No asset processors running");
    return;
  }

  DrawTimeline(painter);
}

void ezQtAssetProcessorProgressWidget::DrawTimeline(QPainter& painter)
{
  ezTime currentTime = ezTime::Now();
  int widgetWidth = width();
  int widgetHeight = height();

  // Calculate timeline dimensions
  int timelineWidth = widgetWidth - s_iLeftMargin - 10;
  double pixelsPerSecond = timelineWidth / m_TimeWindowDuration.GetSeconds();

  // Draw time axis
  painter.setPen(QColor(150, 150, 150));
  int timeAxisY = s_iTopMargin - 10;
  painter.drawLine(s_iLeftMargin, timeAxisY, widgetWidth - 10, timeAxisY);

  // Draw time markers (every 10 seconds)
  QFont timeFont = painter.font();
  timeFont.setPointSize(8);
  painter.setFont(timeFont);

  for (int i = 0; i <= 6; ++i)
  {
    double seconds = i * 10.0;
    int x = widgetWidth - 10 - static_cast<int>(seconds * pixelsPerSecond);
    if (x < s_iLeftMargin)
      break;

    painter.drawLine(x, timeAxisY - 3, x, timeAxisY + 3);
    painter.drawText(x - 15, timeAxisY - 8, 30, 10, Qt::AlignCenter, QString("-%1s").arg(i * 10));
  }

  // Draw each processor row
  for (ezUInt32 i = 0; i < m_uiMaxProcessors; ++i)
  {
    int rowY = s_iTopMargin + i * (s_iRowHeight + s_iRowSpacing);
    DrawProcessorRow(painter, i, rowY, s_iRowHeight, currentTime);
  }
}

void ezQtAssetProcessorProgressWidget::DrawProcessorRow(QPainter& painter, ezUInt32 uiProcessorID, int y, int rowHeight, ezTime currentTime)
{
  int widgetWidth = width();
  int timelineWidth = widgetWidth - s_iLeftMargin - 10;
  double pixelsPerSecond = timelineWidth / m_TimeWindowDuration.GetSeconds();

  // Draw processor label background
  painter.fillRect(0, y, s_iLeftMargin - 5, rowHeight, QColor(60, 60, 65));

  // Draw processor label
  painter.setPen(QColor(200, 200, 200));
  QFont labelFont = painter.font();
  labelFont.setPointSize(9);
  painter.setFont(labelFont);
  painter.drawText(5, y, s_iLeftMargin - 10, rowHeight, Qt::AlignVCenter | Qt::AlignLeft,
    QString("Processor %1").arg(uiProcessorID));

  // Draw row background
  painter.fillRect(s_iLeftMargin, y, timelineWidth, rowHeight, QColor(30, 30, 35));

  // Draw grid lines
  painter.setPen(QPen(QColor(50, 50, 55), 1, Qt::DotLine));
  painter.drawLine(s_iLeftMargin, y + rowHeight, widgetWidth - 10, y + rowHeight);

  // Draw tasks for this processor
  if (uiProcessorID >= m_ProcessorHistory.GetCount())
    return;

  const auto& history = m_ProcessorHistory[uiProcessorID];
  
  for (const auto& task : history)
  {
    ezTime taskStartTime = task.m_StartTime;
    ezTime taskEndTime = task.m_bFinished ? task.m_EndTime : currentTime;

    // Calculate time difference from current time
    double startSecondsAgo = (currentTime - taskStartTime).GetSeconds();
    double endSecondsAgo = (currentTime - taskEndTime).GetSeconds();

    // Skip if outside visible window
    if (startSecondsAgo > m_TimeWindowDuration.GetSeconds())
      continue;

    // Calculate positions
    int endX = widgetWidth - 10 - static_cast<int>(endSecondsAgo * pixelsPerSecond);
    int startX = widgetWidth - 10 - static_cast<int>(startSecondsAgo * pixelsPerSecond);

    // Clamp to visible area
    startX = ezMath::Max(startX, s_iLeftMargin);
    endX = ezMath::Min(endX, widgetWidth - 10);

    if (endX <= s_iLeftMargin)
      continue;

    int barWidth = endX - startX;
    if (barWidth < 1)
      barWidth = 1;

    // Choose color based on status
    QColor barColor;
    if (!task.m_bFinished)
    {
      // Currently processing - animate with a gradient
      barColor = QColor(50, 150, 250); // Blue for in-progress
    }
    else if (task.m_bSuccess)
    {
      barColor = QColor(80, 200, 120); // Green for success
    }
    else
    {
      barColor = QColor(220, 80, 80); // Red for failure
    }

    // Draw task bar
    painter.fillRect(startX, y + 2, barWidth, rowHeight - 4, barColor);

    // Draw border
    painter.setPen(barColor.darker(120));
    painter.drawRect(startX, y + 2, barWidth, rowHeight - 4);

    // Draw asset name if there's enough space
    if (barWidth > 30)
    {
      painter.setPen(QColor(255, 255, 255));
      QFont taskFont = painter.font();
      taskFont.setPointSize(8);
      painter.setFont(taskFont);

      QString shortName = GetShortAssetName(task.m_sAssetPath);
      QFontMetrics fm(taskFont);
      QString elidedName = fm.elidedText(shortName, Qt::ElideRight, barWidth - 6);
      
      painter.drawText(startX + 3, y + 2, barWidth - 6, rowHeight - 4, 
        Qt::AlignVCenter | Qt::AlignLeft, elidedName);
    }
  }
}

QString ezQtAssetProcessorProgressWidget::GetShortAssetName(const ezString& sPath) const
{
  ezStringBuilder path = sPath;
  ezStringView fileName = ezPathUtils::GetFileNameAndExtension(path);
  return QString::fromUtf8(fileName.GetStartPointer(), fileName.GetElementCount());
}

QSize ezQtAssetProcessorProgressWidget::sizeHint() const
{
  int height = s_iTopMargin + (s_iRowHeight + s_iRowSpacing) * m_uiMaxProcessors + s_iRowSpacing;
  return QSize(800, height);
}

QSize ezQtAssetProcessorProgressWidget::minimumSizeHint() const
{
  int height = s_iTopMargin + (s_iRowHeight + s_iRowSpacing) * m_uiMaxProcessors + s_iRowSpacing;
  return QSize(400, ezMath::Max(200, height));
}

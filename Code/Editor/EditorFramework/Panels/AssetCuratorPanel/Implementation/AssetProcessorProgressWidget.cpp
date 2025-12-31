#include "GuiFoundation/Widgets/WidgetUtils.h"


#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Panels/AssetCuratorPanel/AssetProcessorProgressWidget.moc.h>
#include <Foundation/Strings/PathUtils.h>
#include <GuiFoundation/Widgets/GridBarWidget.moc.h>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QTimer>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWheelEvent>

ezQtAssetProcessorProgressWidget::ezQtAssetProcessorProgressWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setMinimumHeight(200);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  setMouseTracking(true);
  setFocusPolicy(Qt::FocusPolicy::ClickFocus);

  // Create clear button
  m_pClearButton = new QPushButton("Clear History", this);
  m_pClearButton->setMaximumWidth(120);
  connect(m_pClearButton, &QPushButton::clicked, this, &ezQtAssetProcessorProgressWidget::OnClearHistory);

  // Connect to asset processor events
  ezAssetProcessor::GetSingleton()->m_ProgressEvents.AddEventHandler(
    ezMakeDelegate(&ezQtAssetProcessorProgressWidget::OnProgressEvent, this));
  ezAssetProcessor::GetSingleton()->m_Events.AddEventHandler(
    ezMakeDelegate(&ezQtAssetProcessorProgressWidget::OnProcessorEvent, this));

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
    ezAssetProcessor::GetSingleton()->m_Events.RemoveEventHandler(
      ezMakeDelegate(&ezQtAssetProcessorProgressWidget::OnProcessorEvent, this));
  }

  if (m_pUpdateTimer)
  {
    m_pUpdateTimer->stop();
  }
}

void ezQtAssetProcessorProgressWidget::SetMaxProcessors(ezUInt32 uiCount)
{
  EZ_LOCK(m_HistoryMutex);
  if (m_uiMaxProcessors == uiCount)
    return;

  m_uiMaxProcessors = uiCount;
  m_ProcessorHistory.SetCount(uiCount);

  QMetaObject::invokeMethod(this, &ezQtAssetProcessorProgressWidget::OnHistoryChanged, Qt::QueuedConnection);
}

void ezQtAssetProcessorProgressWidget::SetGridBarWidget(ezQGridBarWidget* pGridBar)
{
  m_pGridBar = pGridBar;
  OnHistoryChanged();
}

void ezQtAssetProcessorProgressWidget::OnProgressEvent(const ezAssetProcessorProgressEvent& e)
{
  EZ_LOCK(m_HistoryMutex);

  // Make sure we have space for this processor
  if (e.m_uiProcessorID >= m_ProcessorHistory.GetCount())
  {
    SetMaxProcessors(e.m_uiProcessorID + 1);
  }

  auto& history = m_ProcessorHistory[e.m_uiProcessorID];

  if (e.m_Type == ezAssetProcessorProgressEvent::Type::ProcessingStarted)
  {
    if (!m_bCurrentOffsetValid)
    {
      if (m_CurrentOffset.IsZero())
      {
        m_CurrentOffset = e.m_StartTime - (GetLatestTaskTime());
      }
      else
      {
        m_CurrentOffset = e.m_StartTime - (GetLatestTaskTime() + ezTime::MakeFromSeconds(1));
      }
      m_bCurrentOffsetValid = true;
    }

    // Add new task
    ProcessorTask task;
    task.m_AssetGuid = e.m_AssetGuid;
    task.m_sAssetPath = e.m_sAssetPath;
    task.m_StartTime = e.m_StartTime - m_CurrentOffset;
    task.m_TransformState = e.m_TransformState;
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
        history[i].m_EndTime = e.m_EndTime - m_CurrentOffset;
        history[i].m_bFinished = true;
        history[i].m_Result = e.m_Result;
        break;
      }
    }
  }

  QMetaObject::invokeMethod(this, &ezQtAssetProcessorProgressWidget::OnHistoryChanged, Qt::QueuedConnection);
}

void ezQtAssetProcessorProgressWidget::OnUpdateTimer()
{
  EZ_LOCK(m_HistoryMutex);
  if (m_bCurrentOffsetValid)
  {
    // If 1 second has passed and no asset is processing anymore, we cut the timeline here and add a Skip offset. This means that when the next event comes in, we recompute m_CurrentOffset so new transforms are appended right after the previous transforms. Basically we are cutting out downtime out of the graph in which nothing happens so it's nice to scroll through. We indicate that we cut the timeline by drawing a pattern in the background for one second that shows the timeline was cut.
    ezTime currentPos = ezTime::Now() - m_CurrentOffset;
    ezTime latestTaskTime = GetLatestTaskTime();
    if ((currentPos - latestTaskTime) >= ezTime::MakeFromSeconds(1))
    {
      EZ_LOCK(m_HistoryMutex);
      m_bCurrentOffsetValid = false;
      m_SkipOffsets.PushBack(latestTaskTime);
    }
  }
  // Trigger repaint for smooth timeline scrolling
  update();
}

void ezQtAssetProcessorProgressWidget::OnClearHistory()
{
  EZ_LOCK(m_HistoryMutex);

  for (auto& history : m_ProcessorHistory)
  {
    history.Clear();
  }
  OnHistoryChanged();
}
void ezQtAssetProcessorProgressWidget::OnHistoryChanged()
{
  EZ_ASSERT_DEBUG(ezThreadUtils::IsMainThread(), "OnHistoryChanged must be called on the main thread via queued connection");
  EZ_LOCK(m_HistoryMutex);

  {
    // Update widget size based on processor count
    int minHeight = s_iTopMargin + (s_iRowHeight + s_iRowSpacing) * m_uiMaxProcessors + s_iRowSpacing;
    setMinimumHeight(minHeight);
  }

  // Auto-scroll only if the new item doesn't fit in current view
  ezTime latestTime = GetLatestTaskTime();
  ezUInt32 minutes = ezMath::Max(1, ezMath::CeilToInt(latestTime.GetMinutes()));
  ezTime newTimelineLength = ezTime::MakeFromMinutes(minutes);
  if (newTimelineLength != m_TimelineLength)
  {
    m_TimelineLength = newTimelineLength;
    QRectF viewportRect = ComputeViewportSceneRect();
    double latestSeconds = latestTime.GetSeconds();
    double viewportEnd = viewportRect.right();

    // If the new item is outside the visible area, scroll to show it
    if (latestSeconds > viewportEnd)
    {
      m_fSceneTranslationX = latestSeconds - viewportRect.width();
    }
    ClampZoomPan();
    UpdateGridBarConfig();
  }

  update();
}

void ezQtAssetProcessorProgressWidget::OnProcessorEvent(const ezAssetProcessorEvent& e)
{
  if (e.m_Type == ezAssetProcessorEvent::Type::ProcessTaskStateChanged)
  {
    // Clear history when processing stops
    auto state = ezAssetProcessor::GetSingleton()->GetProcessTaskState();
    if (state == ezAssetProcessor::ProcessTaskState::Stopped)
    {
      OnClearHistory();
    }
  }
}

QPoint ezQtAssetProcessorProgressWidget::MapFromScene(const QPointF& pos) const
{
  double x = pos.x() - m_fSceneTranslationX;
  double y = pos.y();
  x *= m_SceneToPixelScale.x();
  y *= m_SceneToPixelScale.y();

  return QPoint(static_cast<int>(x), static_cast<int>(y));
}

QPointF ezQtAssetProcessorProgressWidget::MapToScene(const QPoint& pos) const
{
  double x = pos.x();
  double y = pos.y();
  x /= m_SceneToPixelScale.x();
  y /= m_SceneToPixelScale.y();

  return QPointF(x + m_fSceneTranslationX, y);
}

void ezQtAssetProcessorProgressWidget::ClampZoomPan()
{
  double minPixelPerSecond = width() / m_TimelineLength.GetSeconds();
  m_SceneToPixelScale.setX(ezMath::Clamp(m_SceneToPixelScale.x(), minPixelPerSecond, 200.0)); // 1-200 pixels per second

  double leftLimit = -s_iLeftMargin / m_SceneToPixelScale.x();
  m_fSceneTranslationX = ezMath::Clamp(m_fSceneTranslationX, leftLimit, m_TimelineLength.GetSeconds());
}

void ezQtAssetProcessorProgressWidget::UpdateGridBarConfig()
{
  EZ_ASSERT_DEBUG(ezThreadUtils::IsMainThread(), "");
  if (m_pGridBar == nullptr)
    return;

  QRectF viewportSceneRect = ComputeViewportSceneRect();

  double fFineGridDensity = 0.01;
  double fRoughGridDensity = 0.01;
  ezWidgetUtils::AdjustGridDensity(fFineGridDensity, fRoughGridDensity, rect().width(), viewportSceneRect.width(), 20);

  m_pGridBar->SetConfig(viewportSceneRect, fRoughGridDensity, fFineGridDensity,
    [this](const QPointF& pt) -> QPointF
    {
      return MapFromScene(pt);
    });
}

QRectF ezQtAssetProcessorProgressWidget::ComputeViewportSceneRect() const
{
  int timelineWidth = width();
  double viewWidthSeconds = timelineWidth / m_SceneToPixelScale.x();

  return QRectF(m_fSceneTranslationX, 0, viewWidthSeconds, 1);
}

ezTime ezQtAssetProcessorProgressWidget::GetLatestTaskTime() const
{
  EZ_LOCK(m_HistoryMutex);

  ezTime latest = ezTime::MakeZero();

  for (const auto& history : m_ProcessorHistory)
  {
    if (!history.IsEmpty())
    {
      auto& task = history.PeekBack();
      ezTime taskTime = task.m_bFinished ? task.m_EndTime : (ezTime::Now() - m_CurrentOffset);
      if (taskTime > latest)
      {
        latest = taskTime;
      }
    }
  }

  return latest;
}

const ezQtAssetProcessorProgressWidget::ProcessorTask* ezQtAssetProcessorProgressWidget::FindTaskAtPosition(const QPoint& pos, ezUInt32& out_uiProcessorID) const
{
  EZ_LOCK(m_HistoryMutex);

  // Check if we're in the timeline area
  if (pos.x() < s_iLeftMargin)
    return nullptr;

  // Determine which processor row we're in
  int relativeY = pos.y() - s_iTopMargin;
  if (relativeY < 0)
    return nullptr;

  ezUInt32 uiProcessorID = relativeY / (s_iRowHeight + s_iRowSpacing);
  if (uiProcessorID >= m_uiMaxProcessors || uiProcessorID >= m_ProcessorHistory.GetCount())
    return nullptr;

  // Check if we're within the row bounds (not in the spacing)
  int rowStartY = s_iTopMargin + uiProcessorID * (s_iRowHeight + s_iRowSpacing);
  if (pos.y() < rowStartY || pos.y() > rowStartY + s_iRowHeight)
    return nullptr;

  // Convert mouse position to scene time
  QPointF scenePos = MapToScene(pos);
  double mouseTime = scenePos.x();

  // Search for a task at this position
  const ezDynamicArray<ProcessorTask>& history = m_ProcessorHistory[uiProcessorID];
  const ezTime currentTime = ezTime::Now() - m_CurrentOffset;

  // Use binary search to find the task at mouseTime
  auto it = std::upper_bound(begin(history), end(history), mouseTime,
    [&](double time, const ProcessorTask& task)
    {
      return time < task.m_StartTime.GetSeconds();
    });

  if (it != begin(history))
  {
    --it;
    double startTime = it->m_StartTime.GetSeconds();
    double endTime = it->m_bFinished ? it->m_EndTime.GetSeconds() : currentTime.GetSeconds();

    if (mouseTime >= startTime && mouseTime <= endTime)
    {
      out_uiProcessorID = uiProcessorID;
      return &(*it);
    }
  }

  return nullptr;
}

void ezQtAssetProcessorProgressWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Fill background
  painter.fillRect(rect(), palette().color(QPalette::Window));

  if (m_uiMaxProcessors == 0)
  {
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(rect(), Qt::AlignCenter, "No asset processors running");
    return;
  }

  DrawTimeline(painter);
}

void ezQtAssetProcessorProgressWidget::DrawTimeline(QPainter& painter)
{
  // Note: Time axis is now drawn by the grid bar widget
  // We only draw the processor rows here

  for (ezUInt32 i = 0; i < m_uiMaxProcessors; ++i)
  {
    int rowY = s_iTopMargin + i * (s_iRowHeight + s_iRowSpacing);
    DrawProcessorRow(painter, i, rowY, s_iRowHeight);
  }

  // Position clear button in bottom right corner
  if (m_pClearButton)
  {
    int buttonWidth = m_pClearButton->width();
    int buttonHeight = m_pClearButton->height();
    int x = width() - buttonWidth - 10;
    int y = height() - buttonHeight - 10;
    m_pClearButton->move(x, y);
  }
}

void ezQtAssetProcessorProgressWidget::DrawProcessorRow(QPainter& painter, ezUInt32 uiProcessorID, int y, int rowHeight)
{
  int widgetWidth = width();
  int timelineWidth = widgetWidth - s_iLeftMargin;
  QRectF viewportRect = ComputeViewportSceneRect();

  // Draw processor label background
  painter.fillRect(0, y, s_iLeftMargin - 5, rowHeight, palette().color(QPalette::Window));

  // Draw processor label
  painter.setPen(palette().color(QPalette::Text));
  QFont labelFont = painter.font();
  labelFont.setPointSize(9);
  painter.setFont(labelFont);
  painter.drawText(5, y, s_iLeftMargin - 10, rowHeight, Qt::AlignVCenter | Qt::AlignLeft,
    QString("Processor %1").arg(uiProcessorID));

  // Draw row background
  painter.fillRect(s_iLeftMargin, y, timelineWidth, rowHeight, palette().color(QPalette::Base));

  QPixmap pixmap(16, 16);
  pixmap.fill(Qt::transparent);
  QPainter pixPainter(&pixmap);
  pixPainter.setPen(QPen(palette().color(QPalette::AlternateBase), 5));
  pixPainter.setRenderHint(QPainter::Antialiasing);
  pixPainter.drawLine(-16, -16, 32, 32);
  pixPainter.drawLine(-32, -16, 16, 32);
  pixPainter.drawLine(0, -16, 48, 32);
  pixPainter.end();

  for (const auto& skipOffset : m_SkipOffsets)
  {
    QPoint startPt = MapFromScene(QPointF(skipOffset.GetSeconds(), y));
    QPoint endPt = MapFromScene(QPointF(skipOffset.GetSeconds() + 1, y + rowHeight));
    painter.fillRect(QRectF(startPt, endPt), QBrush(pixmap));
  }

  // Draw tasks for this processor
  EZ_LOCK(m_HistoryMutex);

  if (uiProcessorID >= m_ProcessorHistory.GetCount())
    return;

  const auto& history = m_ProcessorHistory[uiProcessorID];
  ezTime currentTime = ezTime::Now() - m_CurrentOffset;

  for (const auto& task : history)
  {
    ezTime taskStartTime = task.m_StartTime;
    ezTime taskEndTime = task.m_bFinished ? task.m_EndTime : currentTime;

    double startSeconds = taskStartTime.GetSeconds();
    double endSeconds = taskEndTime.GetSeconds();

    // Skip if outside visible window
    if (endSeconds < viewportRect.left() || startSeconds > viewportRect.right())
      continue;

    // Map to screen coordinates
    QPoint startPt = MapFromScene(QPointF(startSeconds, 0));
    QPoint endPt = MapFromScene(QPointF(endSeconds, 0));

    // Clamp to visible area
    int startX = ezMath::Max(startPt.x(), s_iLeftMargin);
    int endX = ezMath::Min(endPt.x(), widgetWidth);

    if (endX <= s_iLeftMargin)
      continue;

    int barWidth = endX - startX;
    if (barWidth < 1)
      barWidth = 1;

    // Choose color based on status
    QColor barColor;
    switch (task.m_TransformState)
    {
      case ezAssetInfo::NeedsTransform:
        barColor = ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Blue));
        break;
      case ezAssetInfo::NeedsThumbnail:
        barColor = ezToQtColor(ezColorScheme::DarkUI(float(ezColorScheme::Blue + ezColorScheme::Green) * 0.5f * ezColorScheme::s_fIndexNormalizer));
        break;
      default:
        barColor = palette().color(QPalette::Highlight);
        break;
    }

    if (!task.m_bFinished)
    {
      // Currently processing - animate with a gradient
      barColor = barColor.darker(100);
    }
    else if (!task.m_Result.Succeeded())
    {
      barColor = ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Red));
    }

    // Draw task bar
    painter.fillRect(startX, y + 2, barWidth, rowHeight - 4, barColor);

    // Draw border
    painter.setPen(barColor.darker(120));
    painter.drawRect(startX, y + 2, barWidth, rowHeight - 4);

    // Draw asset name if there's enough space
    if (barWidth > 30)
    {
      painter.setPen(palette().color(QPalette::BrightText));
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

void ezQtAssetProcessorProgressWidget::mousePressEvent(QMouseEvent* e)
{
  QWidget::mousePressEvent(e);
  m_LastMousePos = e->pos();

  if (m_State != EditState::None)
    return;

  if (e->button() == Qt::RightButton)
  {
    m_State = EditState::Panning;
  }
}

void ezQtAssetProcessorProgressWidget::mouseReleaseEvent(QMouseEvent* e)
{
  QWidget::mouseReleaseEvent(e);

  if (e->button() == Qt::RightButton)
  {
    if (m_State == EditState::Panning)
    {
      m_State = EditState::None;
    }
  }
}

void ezQtAssetProcessorProgressWidget::mouseMoveEvent(QMouseEvent* e)
{
  QWidget::mouseMoveEvent(e);

  const QPoint diff = e->pos() - m_LastMousePos;
  double moveX = static_cast<double>(diff.x()) / m_SceneToPixelScale.x();

  if (m_State == EditState::Panning)
  {
    setCursor(Qt::ClosedHandCursor);
    m_fSceneTranslationX = m_fSceneTranslationX - moveX;
    ClampZoomPan();
    UpdateGridBarConfig();
    update();
  }
  else
  {
    setCursor(Qt::ArrowCursor);

    // Show tooltip when hovering over a task
    ezUInt32 uiProcessorID = 0;
    const ProcessorTask* pTask = FindTaskAtPosition(e->pos(), uiProcessorID);
    if (pTask != nullptr)
    {
      QString tooltip = GetShortAssetName(pTask->m_sAssetPath);
      tooltip += "\n\nPath: " + QString::fromUtf8(pTask->m_sAssetPath.GetData());

      if (pTask->m_bFinished)
      {
        ezTime duration = pTask->m_EndTime - pTask->m_StartTime;
        tooltip += QString("\nDuration: %1s").arg(duration.GetSeconds(), 0, 'f', 3);

        if (pTask->m_Result.Failed() && !pTask->m_Result.m_sMessage.IsEmpty())
        {
          tooltip += "\n\nError:\n" + QString::fromUtf8(pTask->m_Result.m_sMessage.GetData());
        }
      }
      else
      {
        tooltip += "\n\nStatus: Processing...";
      }

      QToolTip::showText(e->globalPos(), tooltip, this);
    }
    else
    {
      QToolTip::hideText();
    }
  }

  m_LastMousePos = e->pos();
}

void ezQtAssetProcessorProgressWidget::wheelEvent(QWheelEvent* e)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  const double ptAtX = MapToScene(mapFromGlobal(e->globalPosition().toPoint())).x();
#else
  const double ptAtX = MapToScene(mapFromGlobal(e->globalPos())).x();
#endif

  double posDiff = m_fSceneTranslationX - ptAtX;
  double changeX = 1.2;

  const double oldScaleX = m_SceneToPixelScale.x();

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  if (e->angleDelta().y() > 0)
#else
  if (e->delta() > 0)
#endif
  {
    m_SceneToPixelScale.setX(m_SceneToPixelScale.x() * changeX);
  }
  else
  {
    m_SceneToPixelScale.setX(m_SceneToPixelScale.x() * (1.0 / changeX));
  }

  ClampZoomPan();

  changeX = m_SceneToPixelScale.x() / oldScaleX;
  posDiff = posDiff * (1.0 / changeX);
  m_fSceneTranslationX = ptAtX + posDiff;

  ClampZoomPan();
  UpdateGridBarConfig();
  update();
}

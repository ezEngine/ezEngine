#include "EditorFramework/Panels/AssetCuratorPanel/AssetProcessorProgressWidget.moc.h"
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

///////////////////////////////////////////
// ezQtAssetProcessorProgressWidget::HistoryState

void ezQtAssetProcessorProgressWidget::HistoryState::SetMaxProcessors(ezUInt32 uiCount)
{
  EZ_LOCK(m_HistoryMutex);
  if (m_ProcessorHistory.GetCount() == uiCount)
    return;

  m_ProcessorHistory.SetCount(uiCount);
  m_ProcessStates.SetCount(uiCount);
}


void ezQtAssetProcessorProgressWidget::HistoryState::ClearHistory()
{
  EZ_LOCK(m_HistoryMutex);

  for (auto& history : m_ProcessorHistory)
  {
    history.Clear();
  }

  m_bCurrentOffsetValid = false;
  m_CurrentOffset = ezTime::MakeZero();
  m_SkipOffsets.Clear();
}

ezTime ezQtAssetProcessorProgressWidget::HistoryState::GetLatestTaskTime() const
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

void ezQtAssetProcessorProgressWidget::HistoryState::OnProgressEvent(const ezAssetProcessorProgressEvent& e)
{
  EZ_LOCK(m_HistoryMutex);

  if (m_pParent == nullptr)
    return;

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

  QMetaObject::invokeMethod(m_pParent, &ezQtAssetProcessorProgressWidget::OnHistoryChanged, Qt::QueuedConnection);
}

void ezQtAssetProcessorProgressWidget::HistoryState::OnProcessorEvent(const ezAssetProcessorEvent& e)
{
  EZ_LOCK(m_HistoryMutex);
  if (m_pParent == nullptr)
    return;

  if (e.m_Type == ezAssetProcessorEvent::Type::ProcessorStateChanged)
  {
    // Clear history when processing stops
    auto state = ezAssetProcessor::GetSingleton()->GetProcessorState();
    if (state == ezAssetProcessor::ProcessorState::Stopped)
    {
      QMetaObject::invokeMethod(m_pParent, &ezQtAssetProcessorProgressWidget::ClearHistory, Qt::QueuedConnection);
    }
    else
    {
      SetMaxProcessors(e.m_uiProcessCount);
      QMetaObject::invokeMethod(m_pParent, &ezQtAssetProcessorProgressWidget::OnHistoryChanged, Qt::QueuedConnection);
    }
  }
  else if (e.m_Type == ezAssetProcessorEvent::Type::ProcessStateChanged)
  {
    QMetaObject::invokeMethod(m_pParent, &ezQtAssetProcessorProgressWidget::OnProcessStateChanged, Qt::QueuedConnection, e.m_uiProcessorID);
  }
}

///////////////////////////////////////////
// ezQtAssetProcessorProgressWidget

ezQtAssetProcessorProgressWidget::ezQtAssetProcessorProgressWidget(QWidget* pParent)
  : QWidget(pParent)
{
  m_pHistoryState = EZ_DEFAULT_NEW(HistoryState);
  m_pHistoryState->m_pParent = this;
  m_pHistoryState->SetMaxProcessors(ezAssetProcessor::GetSingleton()->GetProcessCount());

  setMinimumHeight(200);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  setMouseTracking(true);
  setFocusPolicy(Qt::FocusPolicy::ClickFocus);
  setBackgroundRole(QPalette::Window);

  // Connect to asset processor events
  m_ProgressEventsID = ezAssetProcessor::GetSingleton()->m_ProgressEvents.AddEventHandler([pHistoryState = m_pHistoryState](const ezAssetProcessorProgressEvent& e)
    { pHistoryState->OnProgressEvent(e); });
  m_ProcessorEventsID = ezAssetProcessor::GetSingleton()->m_Events.AddEventHandler([pHistoryState = m_pHistoryState](const ezAssetProcessorEvent& e)
    { pHistoryState->OnProcessorEvent(e); });

  // Setup update timer for smooth animation
  m_pUpdateTimer = new QTimer(this);
  connect(m_pUpdateTimer, &QTimer::timeout, this, &ezQtAssetProcessorProgressWidget::OnUpdateTimer);
  m_pUpdateTimer->start(100); // Update 10 times per second
}

ezQtAssetProcessorProgressWidget::~ezQtAssetProcessorProgressWidget()
{
  if (ezAssetProcessor::GetSingleton())
  {
    ezAssetProcessor::GetSingleton()->m_ProgressEvents.RemoveEventHandler(m_ProgressEventsID);
    ezAssetProcessor::GetSingleton()->m_Events.RemoveEventHandler(m_ProcessorEventsID);
  }

  if (m_pUpdateTimer)
  {
    m_pUpdateTimer->stop();
  }

  EZ_LOCK(m_pHistoryState->m_HistoryMutex);
  m_pHistoryState->m_pParent = nullptr;
}

void ezQtAssetProcessorProgressWidget::SetGridBarWidget(ezQGridBarWidget* pGridBar)
{
  m_pGridBar = pGridBar;
  OnHistoryChanged();
}

void ezQtAssetProcessorProgressWidget::OnUpdateTimer()
{
  EZ_LOCK(m_pHistoryState->m_HistoryMutex);
  if (m_pHistoryState->m_bCurrentOffsetValid)
  {
    // If 1 second has passed and no asset is processing anymore, we cut the timeline here and add a Skip offset. This means that when the next event comes in, we recompute m_CurrentOffset so new transforms are appended right after the previous transforms. Basically we are cutting out downtime out of the graph in which nothing happens so it's nice to scroll through. We indicate that we cut the timeline by drawing a pattern in the background for one second that shows the timeline was cut.
    ezTime currentPos = ezTime::Now() - m_pHistoryState->m_CurrentOffset;
    ezTime latestTaskTime = m_pHistoryState->GetLatestTaskTime();
    if ((currentPos - latestTaskTime) >= ezTime::MakeFromSeconds(1))
    {
      m_pHistoryState->m_bCurrentOffsetValid = false;
      m_pHistoryState->m_SkipOffsets.PushBack(latestTaskTime);
    }
  }
  // Trigger repaint for smooth timeline scrolling
  update();
}

void ezQtAssetProcessorProgressWidget::OnHistoryChanged()
{
  EZ_ASSERT_DEBUG(ezThreadUtils::IsMainThread(), "OnHistoryChanged must be called on the main thread via queued connection");
  EZ_LOCK(m_pHistoryState->m_HistoryMutex);

  if (m_pHistoryState->m_ProcessorHistory.GetCount() > m_uiMaxProcessors)
  {
    // Update widget size based on processor count. We only expand as we don't want to shrink the widget on clear history.
    m_uiMaxProcessors = m_pHistoryState->m_ProcessorHistory.GetCount();
    int minHeight = s_iTopMargin + (s_iRowHeight + s_iRowSpacing) * m_uiMaxProcessors + s_iRowSpacing;
    setMinimumHeight(minHeight);
  }

  // Auto-scroll only if the new item doesn't fit in current view
  ezTime latestTime = m_pHistoryState->GetLatestTaskTime();
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

void ezQtAssetProcessorProgressWidget::OnProcessStateChanged(ezUInt8 uiProcessID)
{
  ezEditorProcessorState state = ezAssetProcessor::GetSingleton()->GetProcessState(uiProcessID);
  {
    EZ_LOCK(m_pHistoryState->m_HistoryMutex);
    if (uiProcessID < m_pHistoryState->m_ProcessStates.GetCount())
    {
      m_pHistoryState->m_ProcessStates[uiProcessID] = state;
    }
  }
  update();
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

void ezQtAssetProcessorProgressWidget::ClearHistory()
{
  EZ_ASSERT_DEBUG(ezThreadUtils::IsMainThread(), "ClearHistory must be called on the main thread via queued connection");
  m_pHistoryState->ClearHistory();
  m_TimelineLength = ezTime::MakeZero();
  m_fSceneTranslationX = -100.0;
}

void ezQtAssetProcessorProgressWidget::ClampZoomPan()
{
  double minPixelPerSecond = width() / m_TimelineLength.GetSeconds();
  m_SceneToPixelScale.setX(ezMath::Clamp(m_SceneToPixelScale.x(), minPixelPerSecond, 200.0)); // 1-200 pixels per second

  double leftLimit = -s_iLeftMargin / m_SceneToPixelScale.x();
  m_fSceneTranslationX = ezMath::Clamp(m_fSceneTranslationX, leftLimit, m_TimelineLength.GetSeconds());
}

void ezQtAssetProcessorProgressWidget::UpdateGridBarConfig() const
{
  EZ_ASSERT_DEBUG(ezThreadUtils::IsMainThread(), "UpdateGridBarConfig must be called from the main thread.");
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

const ezQtAssetProcessorProgressWidget::ProcessorTask* ezQtAssetProcessorProgressWidget::FindTaskAtPosition(const QPoint& pos, ezUInt32& out_uiProcessorID) const
{
  EZ_LOCK(m_pHistoryState->m_HistoryMutex);
  out_uiProcessorID = ezInvalidIndex;

  // Determine which processor row we're in
  int relativeY = pos.y() - s_iTopMargin;
  if (relativeY < 0)
    return nullptr;

  ezUInt32 uiProcessorID = relativeY / (s_iRowHeight + s_iRowSpacing);
  if (uiProcessorID >= m_pHistoryState->m_ProcessorHistory.GetCount())
    return nullptr;

  // Check if we're within the row bounds (not in the spacing)
  int rowStartY = s_iTopMargin + uiProcessorID * (s_iRowHeight + s_iRowSpacing);
  if (pos.y() < rowStartY || pos.y() > rowStartY + s_iRowHeight)
    return nullptr;

  out_uiProcessorID = uiProcessorID;
  // Check if we're in the timeline area
  if (pos.x() < s_iLeftMargin)
    return nullptr;

  // Convert mouse position to scene time
  QPointF scenePos = MapToScene(pos);
  double mouseTime = scenePos.x();

  // Search for a task at this position
  const ezDynamicArray<ProcessorTask>& history = m_pHistoryState->m_ProcessorHistory[uiProcessorID];
  const ezTime currentTime = ezTime::Now() - m_pHistoryState->m_CurrentOffset;

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
  // painter.setRenderHint(QPainter::Antialiasing);
  DrawTimeline(painter);
}

void ezQtAssetProcessorProgressWidget::DrawTimeline(QPainter& painter)
{
  for (ezUInt32 i = 0; i < m_uiMaxProcessors; ++i)
  {
    int rowY = s_iTopMargin + i * (s_iRowHeight + s_iRowSpacing);
    DrawProcessorRow(painter, i, rowY, s_iRowHeight);
  }
}

void ezQtAssetProcessorProgressWidget::DrawProcessorRow(QPainter& painter, ezUInt32 uiProcessorID, int y, int rowHeight)
{
  const int widgetWidth = width();
  const int timelineWidth = widgetWidth - s_iLeftMargin;
  const QRectF viewportRect = ComputeViewportSceneRect();

  EZ_LOCK(m_pHistoryState->m_HistoryMutex);
  const ezEditorProcessorState state = m_pHistoryState->m_ProcessStates[uiProcessorID];

  // Draw processor label background
  painter.fillRect(0, y, s_iLeftMargin - 5, rowHeight, palette().color(QPalette::Window));

  // Draw state indicator
  {
    const int indicatorX = 5;
    const int indicatorY = y + (rowHeight - 10) / 2;
    QColor indicatorColor = palette().color(QPalette::Mid);
    if (state.m_bConnected)
    {
      indicatorColor = state.m_bRunning ? ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Green)) : ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Yellow));
    }
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(indicatorColor);
    painter.setPen(indicatorColor.darker(150));
    painter.drawEllipse(indicatorX, indicatorY, s_iIndicatorSize, s_iIndicatorSize);
    painter.setBrush(Qt::NoBrush);
    painter.setRenderHint(QPainter::Antialiasing, false);
  }

  // Draw processor label
  {
    painter.setPen(palette().color(QPalette::Text));
    QFont labelFont = painter.font();
    labelFont.setPointSize(9);
    painter.setFont(labelFont);
    painter.drawText(5 + s_iIndicatorSize + 8, y, s_iLeftMargin - 23 - s_iIndicatorSize, rowHeight, Qt::AlignVCenter | Qt::AlignLeft,
      QString("Process %1").arg(uiProcessorID));
  }

  // Draw row background
  painter.fillRect(s_iLeftMargin, y, timelineWidth, rowHeight, palette().color(QPalette::Base));

  // Draw skip offsets
  {
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter pixPainter(&pixmap);
    pixPainter.setPen(QPen(palette().color(QPalette::AlternateBase), 5));
    pixPainter.drawLine(-16, -16, 32, 32);
    pixPainter.drawLine(-32, -16, 16, 32);
    pixPainter.drawLine(0, -16, 48, 32);
    pixPainter.end();

    for (const auto& skipOffset : m_pHistoryState->m_SkipOffsets)
    {
      QPoint startPt = MapFromScene(QPointF(skipOffset.GetSeconds(), y));
      if (startPt.x() < s_iLeftMargin)
      {
        startPt.setX(s_iLeftMargin);
      }
      QPoint endPt = MapFromScene(QPointF(skipOffset.GetSeconds() + 1, y + rowHeight));
      if (startPt.x() < endPt.x())
        painter.fillRect(QRectF(startPt, endPt), QBrush(pixmap));
    }
  }

  // Draw tasks for this processor
  if (uiProcessorID >= m_pHistoryState->m_ProcessorHistory.GetCount())
    return;

  const auto& history = m_pHistoryState->m_ProcessorHistory[uiProcessorID];
  ezTime currentTime = ezTime::Now() - m_pHistoryState->m_CurrentOffset;

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

      ezStringView fileName = ezPathUtils::GetFileNameAndExtension(task.m_sAssetPath);
      QString shortName = ezMakeQString(fileName);
      QFontMetrics fm(taskFont);
      QString elidedName = fm.elidedText(shortName, Qt::ElideRight, barWidth - 6);

      painter.drawText(startX + 3, y + 2, barWidth - 6, rowHeight - 4,
        Qt::AlignVCenter | Qt::AlignLeft, elidedName);
    }
  }
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

  if (m_EditState != EditState::None)
    return;

  if (e->button() == Qt::RightButton)
  {
    m_EditState = EditState::Panning;
  }
}

void ezQtAssetProcessorProgressWidget::mouseReleaseEvent(QMouseEvent* e)
{
  QWidget::mouseReleaseEvent(e);

  if (e->button() == Qt::RightButton)
  {
    if (m_EditState == EditState::Panning)
    {
      m_EditState = EditState::None;
    }
  }
}

void ezQtAssetProcessorProgressWidget::ShowTooltip(QMouseEvent* e)
{
  EZ_LOCK(m_pHistoryState->m_HistoryMutex);

  ezUInt32 uiProcessorID = 0;
  const ProcessorTask* pTask = FindTaskAtPosition(e->pos(), uiProcessorID);

  if (e->pos().x() < s_iLeftMargin && uiProcessorID != ezInvalidIndex)
  {
    ezEditorProcessorState state = m_pHistoryState->m_ProcessStates[uiProcessorID];
    ezStringBuilder sTooltip;
    sTooltip.SetFormat("ezEditorProcessor {}\nProcessID: {}\nConnected: {}\nRunning: {}", uiProcessorID, state.m_uiProcessID, state.m_bConnected, state.m_bRunning);
    QToolTip::showText(e->globalPos(), ezMakeQString(sTooltip), this);
    return;
  }

  if (pTask == nullptr)
  {
    QToolTip::hideText();
    return;
  }

  ezStringView sFileName = ezPathUtils::GetFileNameAndExtension(pTask->m_sAssetPath);
  ezStringBuilder sTooltip;
  sTooltip.SetFormat("{}\n\nPath: {}", sFileName, pTask->m_sAssetPath);
  if (pTask->m_bFinished)
  {
    ezTime duration = pTask->m_EndTime - pTask->m_StartTime;
    sTooltip.AppendFormat("\n{} duration: {}s", pTask->m_TransformState == ezAssetInfo::TransformState::NeedsTransform ? "Transform" : "Thumbnail", ezArgF(duration.GetSeconds(), 3));

    if (pTask->m_Result.Failed())
    {
      sTooltip.AppendFormat("\n\nError: {}", pTask->m_Result.m_sMessage);
    }
  }
  else
  {
    sTooltip.Append("\n\nStatus: Processing...");
  }

  QToolTip::showText(e->globalPos(), ezMakeQString(sTooltip), this);
}

void ezQtAssetProcessorProgressWidget::mouseMoveEvent(QMouseEvent* e)
{
  QWidget::mouseMoveEvent(e);

  const QPoint diff = e->pos() - m_LastMousePos;
  double moveX = static_cast<double>(diff.x()) / m_SceneToPixelScale.x();

  if (m_EditState == EditState::Panning)
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
    ShowTooltip(e);
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

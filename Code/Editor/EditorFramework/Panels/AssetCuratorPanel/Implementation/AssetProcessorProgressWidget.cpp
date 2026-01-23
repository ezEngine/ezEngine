#include <EditorFramework/Panels/AssetCuratorPanel/AssetProcessorProgressWidget.moc.h>

#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/EditorFrameworkPCH.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Time/Stopwatch.h>
#include <GuiFoundation/Widgets/GridBarWidget.moc.h>
#include <GuiFoundation/Widgets/WidgetUtils.h>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QToolTip>
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

  m_FailedTransforms.Clear();
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
      ezTime taskTime = task.IsFinished() ? ezTime::MakeFromSeconds(task.EndTime()) : (ezTime::Now() - m_CurrentOffset);
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
      // Compute a new offset since we just started processing assets agan after a longer period of downtime.
      if (m_CurrentOffset.IsZero())
      {
        // Special case for the very first asset im the timeline.
        m_CurrentOffset = e.m_StartTime - (GetLatestTaskTime());
      }
      else
      {
        // We draw a 1-second area whenever we squish downtime. So the new offset must start one second later.
        m_CurrentOffset = e.m_StartTime - (GetLatestTaskTime() + ezTime::MakeFromSeconds(1));
      }
      m_bCurrentOffsetValid = true;
    }

    // Add new task
    ProcessorTask task;
    task.m_AssetGuid = e.m_AssetGuid;
    task.m_fStartTimeInSeconds = static_cast<float>((e.m_StartTime - m_CurrentOffset).GetSeconds());
    task.m_fTransformStartTimeInSeconds = task.m_fStartTimeInSeconds;
    task.m_TransformState = e.m_TransformState;
    history.PushBack(task);
  }
  else if (e.m_Type == ezAssetProcessorProgressEvent::Type::ProcessingFinished)
  {
    ProcessorTask& task = history.PeekBack();
    EZ_ASSERT_DEBUG(task.m_AssetGuid == e.m_AssetGuid, "Processing finished should always map to the previously started asset");
    EZ_ASSERT_DEBUG(!task.IsFinished(), "Last task should not be finished if ProcessingFinished is emitted");
    task.m_fDurationInSeconds = static_cast<float>((e.m_EndTime - e.m_StartTime).GetSeconds());
    task.m_fTransformStartTimeInSeconds = static_cast<float>((e.m_TransformStartTime - m_CurrentOffset).GetSeconds());
    if (e.m_Result.Failed())
    {
      m_FailedTransforms.PushBack(e.m_Result);
      task.m_uiResultIndex = m_FailedTransforms.GetCount() - 1;
    }
  }
  QMetaObject::invokeMethod(m_pParent, &ezQtAssetProcessorProgressWidget::OnHistoryChanged, Qt::QueuedConnection);
}

void ezQtAssetProcessorProgressWidget::HistoryState::OnProcessorEvent(const ezAssetProcessorEvent& e)
{
  EZ_LOCK(m_HistoryMutex);
  if (m_pParent == nullptr)
    return;

  if (e.m_Type == ezAssetProcessorEvent::Type::AssetProcessorStateChanged)
  {
    // Clear history when processing stops
    auto state = ezAssetProcessor::GetSingleton()->GetProcessorState();
    if (state == ezAssetProcessor::ProcessorState::Running)
    {
      SetMaxProcessors(e.m_uiProcessCount);
    }
    QMetaObject::invokeMethod(m_pParent, &ezQtAssetProcessorProgressWidget::OnProcessorStateChanged, Qt::QueuedConnection);
  }
  else if (e.m_Type == ezAssetProcessorEvent::Type::ProcessStateChanged)
  {
    const ezUInt8 uiProcId = e.m_uiProcessorID;
    QMetaObject::invokeMethod(m_pParent, [p = m_pParent, uiProcId]()
      { p->OnProcessStateChanged(uiProcId); }, Qt::QueuedConnection);
  }
}

const ezQtAssetProcessorProgressWidget::ProcessorTask* ezQtAssetProcessorProgressWidget::HistoryState::FindTaskAtTime(ezUInt32 uiProcessorID, double fPointInTimeSec) const
{
  EZ_LOCK(m_HistoryMutex);

  const ezDynamicArray<ProcessorTask>& history = m_ProcessorHistory[uiProcessorID];
  const float fCurrentTime = static_cast<float>((ezTime::Now() - m_CurrentOffset).GetSeconds());

  // Use binary search to find the task at pointInTime
  auto it = std::upper_bound(begin(history), end(history), fPointInTimeSec,
    [&](double time, const ProcessorTask& task)
    {
      return time < task.m_fStartTimeInSeconds;
    });

  if (it != begin(history))
  {
    --it;
    float fStartTime = it->m_fStartTimeInSeconds;
    float fEndTime = it->IsFinished() ? it->EndTime() : fCurrentTime;

    if (fPointInTimeSec >= fStartTime && fPointInTimeSec <= fEndTime)
    {
      return &(*it);
    }
  }

  return nullptr;
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

  // Setup update timer
  m_pUpdateTimer = new QTimer(this);
  connect(m_pUpdateTimer, &QTimer::timeout, this, &ezQtAssetProcessorProgressWidget::OnUpdateTimer);
  m_pUpdateTimer->start(100);
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

void ezQtAssetProcessorProgressWidget::InitializePaintCaches() const
{
  if (m_bCachesInitialized)
    return;

  m_bCachesInitialized = true;

  // Initialize fonts
  m_ProcessorLabelFont = font();
  m_ProcessorLabelFont.setPointSize(9);

  m_TaskLabelFont = font();
  m_TaskLabelFont.setPointSize(8);

  // Create skip offset pattern pixmap
  m_SkipOffsetPattern = QPixmap(16, 16);
  m_SkipOffsetPattern.fill(Qt::transparent);
  QPainter pixPainter(&m_SkipOffsetPattern);
  pixPainter.setPen(QPen(palette().color(QPalette::AlternateBase), 5));
  pixPainter.drawLine(-16, -16, 32, 32);
  pixPainter.drawLine(-32, -16, 16, 32);
  pixPainter.drawLine(0, -16, 48, 32);
  pixPainter.end();

  // Cache task colors
  m_NeedsTransformColor[0] = ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Blue));
  m_NeedsTransformColor[1] = m_NeedsTransformColor[0].darker(150);
  m_NeedsThumbnailColor[0] = ezToQtColor(ezColorScheme::DarkUI(float(ezColorScheme::Blue + ezColorScheme::Green) * 0.5f * ezColorScheme::s_fIndexNormalizer));
  m_NeedsThumbnailColor[1] = m_NeedsThumbnailColor[0].darker(150);
  m_ErrorColor[0] = ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Red));
  m_ErrorColor[1] = m_ErrorColor[0].darker(150);
}

void ezQtAssetProcessorProgressWidget::InvalidatePaintCaches()
{
  m_bCachesInitialized = false;
  m_ProcessorLabels.Clear();
}

void ezQtAssetProcessorProgressWidget::SetGridBarWidget(ezQGridBarWidget* pGridBar)
{
  EZ_ASSERT_DEBUG(m_pGridBar == nullptr, "SetGridBarWidget should only be called once.");
  m_pGridBar = pGridBar;
  ClampZoomPan();
  UpdateGridBarConfig();
  OnHistoryChanged();
}

void ezQtAssetProcessorProgressWidget::SetScrollBarWidget(QScrollBar* pScrollBar)
{
  EZ_ASSERT_DEBUG(m_pScrollBar == nullptr, "SetScrollBarWidget should only be called once.");
  m_pScrollBar = pScrollBar;

  // When the scrollbar value changes, it provides milliseconds. Convert to seconds.
  connect(m_pScrollBar, &QScrollBar::valueChanged, this, [this](int v)
    {
      m_fSceneTranslationX = static_cast<double>(v) / 1000.0;
      ClampZoomPan();
      UpdateGridBarConfig();
      update(); //
    });
  connect(m_pScrollBar, &QScrollBar::sliderReleased, this, [this]()
    { m_AssetNameCache.Clear(); });

  ClampZoomPan();
  UpdateGridBarConfig();
  OnHistoryChanged();
}

void ezQtAssetProcessorProgressWidget::ClearHistory()
{
  EZ_ASSERT_DEBUG(ezThreadUtils::IsMainThread(), "ClearHistory must be called on the main thread via queued connection");
  m_pHistoryState->ClearHistory();
  m_TimelineLength = ezTime::MakeFromMinutes(1);
  m_fSceneTranslationX = 0;
  ClampZoomPan();
  UpdateGridBarConfig();
  update();
}

void ezQtAssetProcessorProgressWidget::paintEvent(QPaintEvent* event)
{
  EZ_PROFILE_SCOPE("ezQtAssetProcessorProgressWidget::paintEvent");

  // Initialize caches on first paint
  InitializePaintCaches();

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, false);
  DrawTimeline(painter);
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
    ShowTooltip(e);
  }

  m_LastMousePos = e->pos();
}

void ezQtAssetProcessorProgressWidget::mouseReleaseEvent(QMouseEvent* e)
{
  QWidget::mouseReleaseEvent(e);

  if (e->button() == Qt::RightButton)
  {
    if (m_EditState == EditState::Panning)
    {
      m_AssetNameCache.Clear();
      m_EditState = EditState::None;
    }
  }
}

void ezQtAssetProcessorProgressWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
  QWidget::mouseDoubleClickEvent(e);

  EZ_LOCK(m_pHistoryState->m_HistoryMutex);

  ezUInt32 uiProcessorID = 0;
  const ProcessorTask* pTask = FindTaskAtPosition(e->pos(), uiProcessorID);

  if (pTask == nullptr)
    return;

  // If clicked on the processor label area, do nothing
  if (e->pos().x() < s_iLeftMargin)
    return;

  const ezString& sAssetPath = GetAssetPath(pTask->m_AssetGuid);
  ezQtEditorApp::GetSingleton()->OpenDocumentQueued(sAssetPath);
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

  // Clamp zoom first before computing offset or zooming at max-zoom will cause drift.
  ClampZoomPan();
  changeX = m_SceneToPixelScale.x() / oldScaleX;
  posDiff = posDiff * (1.0 / changeX);
  m_fSceneTranslationX = ptAtX + posDiff;
  m_AssetNameCache.Clear();

  ClampZoomPan();
  UpdateGridBarConfig();
  update();
}

void ezQtAssetProcessorProgressWidget::changeEvent(QEvent* e)
{
  QWidget::changeEvent(e);

  if (e->type() == QEvent::PaletteChange || e->type() == QEvent::StyleChange)
  {
    InvalidatePaintCaches();
  }
}

QSize ezQtAssetProcessorProgressWidget::sizeHint() const
{
  int height = s_iTopMargin + (s_iRowHeight + s_iRowSpacing) * m_uiMaxProcessors;
  return QSize(800, height);
}

QSize ezQtAssetProcessorProgressWidget::minimumSizeHint() const
{
  int height = s_iTopMargin + (s_iRowHeight + s_iRowSpacing) * m_uiMaxProcessors;
  return QSize(400, ezMath::Max(200, height));
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
    // Trigger repaint for smooth timeline scrolling
    update();
  }
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

  // Extend timeline length
  ezTime latestTime = m_pHistoryState->GetLatestTaskTime();
  ezUInt32 minutes = ezMath::Max(1, ezMath::CeilToInt(latestTime.GetMinutes()));
  ezTime newTimelineLength = ezTime::MakeFromMinutes(minutes);
  if (newTimelineLength != m_TimelineLength)
  {
    m_TimelineLength = newTimelineLength;
    ClampZoomPan();
    UpdateGridBarConfig();
  }
}

void ezQtAssetProcessorProgressWidget::OnProcessorStateChanged()
{
  OnHistoryChanged();
  for (ezUInt32 i = 0; i < m_uiMaxProcessors; ++i)
  {
    OnProcessStateChanged(i);
  }
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

  return QPoint(static_cast<int>(x) + s_iLeftMargin, static_cast<int>(y));
}

QPointF ezQtAssetProcessorProgressWidget::MapToScene(const QPoint& pos) const
{
  double x = pos.x() - s_iLeftMargin;
  double y = pos.y();
  x /= m_SceneToPixelScale.x();
  y /= m_SceneToPixelScale.y();

  return QPointF(x + m_fSceneTranslationX, y);
}

QRectF ezQtAssetProcessorProgressWidget::ComputeViewportSceneRect() const
{
  int timelineWidth = width();
  double viewWidthSeconds = timelineWidth / m_SceneToPixelScale.x();

  double leftLimit = s_iLeftMargin / m_SceneToPixelScale.x();
  return QRectF(m_fSceneTranslationX - leftLimit, 0, viewWidthSeconds, 1);
}

void ezQtAssetProcessorProgressWidget::ClampZoomPan()
{
  double minPixelPerSecond = width() / m_TimelineLength.GetSeconds();
  m_SceneToPixelScale.setX(ezMath::Clamp(m_SceneToPixelScale.x(), minPixelPerSecond, 500.0));
  m_fSceneTranslationX = ezMath::Clamp(m_fSceneTranslationX, 0.0, m_TimelineLength.GetSeconds());
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

  if (m_pScrollBar == nullptr)
    return;

  ezQtScopedBlockSignals bs(m_pScrollBar);
  m_pScrollBar->setMinimum(0);
  m_pScrollBar->setMaximum((int)m_TimelineLength.GetMilliseconds());
  m_pScrollBar->setSliderPosition((int)(m_fSceneTranslationX * 1000.0));
  m_pScrollBar->setSingleStep(100);
  m_pScrollBar->setPageStep(10000);
}

const ezString& ezQtAssetProcessorProgressWidget::GetAssetPath(const ezUuid& assetGuid) const
{
  if (auto it = m_AssetNameCache.Find(assetGuid); it.IsValid())
  {
    return it.Value();
  }

  if (auto asset = ezAssetCurator::GetSingleton()->GetSubAsset(assetGuid); asset.isValid())
  {
    auto it = m_AssetNameCache.Insert(assetGuid, asset->m_pAssetInfo->m_Path.GetAbsolutePath());
    return it.Value();
  }

  return m_sUnknownAsset;
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

  return m_pHistoryState->FindTaskAtTime(uiProcessorID, mouseTime);
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
    QToolTip::showText(e->globalPosition().toPoint(), ezMakeQString(sTooltip), this);
    return;
  }

  if (pTask == nullptr)
  {
    QToolTip::hideText();
    return;
  }

  ezStringView sFileName = ezPathUtils::GetFileNameAndExtension(GetAssetPath(pTask->m_AssetGuid));
  ezStringBuilder sTooltip;
  sTooltip.SetFormat("{}\n\nPath: {}", sFileName, GetAssetPath(pTask->m_AssetGuid));
  if (pTask->IsFinished())
  {
    sTooltip.AppendFormat("\n{} duration: {}s", pTask->m_TransformState == ezAssetInfo::TransformState::NeedsTransform ? "Transform" : "Thumbnail", ezArgF(pTask->m_fDurationInSeconds, 3));
    sTooltip.AppendFormat("\nTime spent on curator updates: {}s", ezArgF(pTask->m_fTransformStartTimeInSeconds - pTask->m_fStartTimeInSeconds, 3));

    if (pTask->Failed())
    {
      sTooltip.AppendFormat("\n\nError: {}", m_pHistoryState->m_FailedTransforms[pTask->m_uiResultIndex].m_sMessage);
    }
  }
  else
  {
    sTooltip.Append("\n\nStatus: Processing...");
  }

  QToolTip::showText(e->globalPosition().toPoint(), ezMakeQString(sTooltip), this);
}

void ezQtAssetProcessorProgressWidget::DrawTimeline(QPainter& painter) const
{
  for (ezUInt32 i = 0; i < m_uiMaxProcessors; ++i)
  {
    int rowY = s_iTopMargin + s_iRowSpacing + i * (s_iRowHeight + s_iRowSpacing);
    DrawProcessorRow(painter, i, rowY);
  }
}

void ezQtAssetProcessorProgressWidget::DrawProcessorRow(QPainter& painter, ezUInt32 uiProcessorID, int y) const
{
  const int widgetWidth = width();
  const int timelineWidth = widgetWidth - s_iLeftMargin;
  const QRectF viewportRect = ComputeViewportSceneRect();

  // Copy state data and history under lock, then draw without lock
  ezEditorProcessorState state;
  ezDynamicArray<ProcessorTask> visibleTasks;
  ezHybridArray<ezTime, 8> skipOffsets;
  ezTime currentTime;

  {
    EZ_LOCK(m_pHistoryState->m_HistoryMutex);

    if (uiProcessorID < m_pHistoryState->m_ProcessStates.GetCount())
    {
      state = m_pHistoryState->m_ProcessStates[uiProcessorID];
    }

    skipOffsets = m_pHistoryState->m_SkipOffsets;
    currentTime = ezTime::Now() - m_pHistoryState->m_CurrentOffset;

    // Copy only visible tasks
    if (uiProcessorID < m_pHistoryState->m_ProcessorHistory.GetCount())
    {
      const auto& history = m_pHistoryState->m_ProcessorHistory[uiProcessorID];
      visibleTasks.Reserve(history.GetCount());
      const float fStartTimeView = static_cast<float>(MapToScene({s_iLeftMargin, 0}).x());
      const float fCurrentTimeSeconds = static_cast<float>(currentTime.GetSeconds());

      auto it = std::upper_bound(begin(history), end(history), fStartTimeView,
        [&](float time, const ProcessorTask& task)
        {
          return time < task.m_fStartTimeInSeconds;
        });
      if (it != begin(history))
        it--;

      for (; it != end(history); ++it)
      {
        const ProcessorTask& task = *it;
        float fTaskEndTime = task.IsFinished() ? task.EndTime() : fCurrentTimeSeconds;

        if (fTaskEndTime < viewportRect.left())
          continue;
        if (task.m_fStartTimeInSeconds > viewportRect.right())
          break;

        visibleTasks.PushBack(task);
      }
    }
  }

  // Draw processor label background
  painter.fillRect(0, y, s_iLeftMargin - 5, s_iRowHeight, palette().color(QPalette::Window));

  // Draw state indicator
  {
    const int indicatorX = 5;
    const int indicatorY = y + (s_iRowHeight - 10) / 2;
    QColor indicatorColor = palette().color(QPalette::Mid);
    if (state.m_bConnected)
    {
      indicatorColor = state.m_bRunning ? ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Green)) : ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Yellow));
    }
    painter.setBrush(indicatorColor);
    painter.setPen(indicatorColor.darker(150));
    painter.drawEllipse(indicatorX, indicatorY, s_iIndicatorSize, s_iIndicatorSize);
    painter.setBrush(Qt::NoBrush);
  }

  // Draw processor label
  {
    painter.setPen(palette().color(QPalette::Text));
    painter.setFont(m_ProcessorLabelFont);

    // Ensure we have a cached label for this processor
    while (m_ProcessorLabels.GetCount() <= uiProcessorID)
    {
      m_ProcessorLabels.PushBack(QString("Process %1").arg(m_ProcessorLabels.GetCount()));
    }

    painter.drawText(5 + s_iIndicatorSize + 8, y, s_iLeftMargin - 23 - s_iIndicatorSize, s_iRowHeight, Qt::AlignVCenter | Qt::AlignLeft,
      m_ProcessorLabels[uiProcessorID]);
  }

  // Draw row background
  painter.fillRect(s_iLeftMargin, y, timelineWidth, s_iRowHeight, palette().color(QPalette::Base));

  // Draw skip offsets
  {
    for (const auto& skipOffset : skipOffsets)
    {
      QPoint startPt = MapFromScene(QPointF(skipOffset.GetSeconds(), y));
      if (startPt.x() < s_iLeftMargin)
      {
        startPt.setX(s_iLeftMargin);
      }
      QPoint endPt = MapFromScene(QPointF(skipOffset.GetSeconds() + 1, y + s_iRowHeight));
      if (startPt.x() < endPt.x())
        painter.fillRect(QRectF(startPt, endPt), QBrush(m_SkipOffsetPattern));
    }
  }

  // Draw tasks for this processor
  for (const ProcessorTask& task : visibleTasks)
  {
    float fTaskStartTime = task.m_fStartTimeInSeconds;
    float fTaskEndTime = task.IsFinished() ? task.EndTime() : static_cast<float>(currentTime.GetSeconds());

    // Map to screen coordinates
    QPoint startPt = MapFromScene(QPointF(fTaskStartTime, y + 2));
    QPoint transformPt = MapFromScene(QPointF(task.m_fTransformStartTimeInSeconds, y + 2));
    QPoint endPt = MapFromScene(QPointF(fTaskEndTime, y + s_iRowHeight - 4));
    startPt.setX(ezMath::Max(startPt.x(), s_iLeftMargin));
    transformPt.setX(ezMath::Max(transformPt.x(), s_iLeftMargin));
    endPt.setX(ezMath::Min(endPt.x(), widgetWidth));
    if (startPt.x() > endPt.x())
      continue;

    if (endPt.x() == startPt.x())
      endPt.setX(endPt.x() + 1);

    const QRect rect(startPt, endPt);
    const QRect actualWork(transformPt, endPt);
    DrawProcessorTask(painter, task, rect, actualWork);
  }
}

void ezQtAssetProcessorProgressWidget::DrawProcessorTask(QPainter& painter, const ProcessorTask& task, const QRect& rect, const QRect& actualWork) const
{
  // Choose color based on status
  QColor barColor;
  QColor barColorDarker;
  switch (task.m_TransformState)
  {
    case ezAssetInfo::NeedsTransform:
      barColor = m_NeedsTransformColor[0];
      barColorDarker = m_NeedsTransformColor[1];
      break;
    case ezAssetInfo::NeedsThumbnail:
      barColor = m_NeedsThumbnailColor[0];
      barColorDarker = m_NeedsThumbnailColor[1];
      break;
    default:
      barColor = palette().color(QPalette::Highlight);
      barColorDarker = barColor.darker(150);
      break;
  }
  if (task.Failed())
  {
    barColor = m_ErrorColor[0];
    barColorDarker = m_ErrorColor[1];
  }

  // Draw task bar. The rect is over the entire task runtime
  painter.fillRect(rect, barColorDarker);
  // The actual work is when the task was actually processing instead of checking the asset curator for up-to-date info. We adjust the rect so we get a convenient border from the rect above.
  painter.fillRect(actualWork.adjusted(1, 1, -1, -1), task.IsFinished() ? barColor : barColorDarker);

  // Draw asset name if there's enough space
  int barWidth = rect.width();
  if (barWidth > 30)
  {
    painter.setPen(palette().color(QPalette::BrightText));
    painter.setFont(m_TaskLabelFont);

    ezStringView fileName = ezPathUtils::GetFileNameAndExtension(GetAssetPath(task.m_AssetGuid));
    QString shortName = ezMakeQString(fileName);
    QFontMetrics fm(m_TaskLabelFont);
    QString elidedName = fm.elidedText(shortName, Qt::ElideRight, barWidth - 6);
    QRect textRect = rect.adjusted(3, 0, -3, 0);
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedName);
  }
}

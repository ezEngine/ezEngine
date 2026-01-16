#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/Declarations.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>
#include <QWidget>

class ezQGridBarWidget;
struct ezAssetProcessorProgressEvent;
struct ezAssetProcessorEvent;
class QPushButton;
class QScrollBar;

/// \brief Visual progress widget that displays an asset processing timeline.
///
/// Shows a timeline view with Y-axis representing each ezEditorProcessor and X-axis representing time.
/// Each asset being processed is displayed as a bar on the timeline.
/// The timeline is compacted in that every length of time in which no asset processing happens is reduced to one second in the timeline and a diagonal pattern is drawn to indicate that the timeline cuts off there.
class EZ_EDITORFRAMEWORK_DLL ezQtAssetProcessorProgressWidget : public QWidget
{
  Q_OBJECT

public:
  explicit ezQtAssetProcessorProgressWidget(QWidget* pParent = nullptr);
  ~ezQtAssetProcessorProgressWidget();

  /// \brief An ezQGridBarWidget must be placed over this widget in a vertical layout and then set here.
  void SetGridBarWidget(ezQGridBarWidget* pGridBar);
  void SetScrollBarWidget(QScrollBar* pScrollBar);

public Q_SLOTS:
  void ClearHistory();

protected:
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  virtual QSize sizeHint() const override;
  virtual QSize minimumSizeHint() const override;

private:
  enum class EditState
  {
    None,
    Panning
  };

  /// \brief Compact representation of an item in the timeline of the asset processor.
  struct ProcessorTask
  {
    static constexpr ezUInt16 SuccessResult = 0xFFFF;
    EZ_ALWAYS_INLINE bool IsFinished() const { return m_fDurationInSeconds != -1; }
    EZ_ALWAYS_INLINE bool Failed() const { return m_uiResultIndex != SuccessResult; }
    EZ_ALWAYS_INLINE ezTime EndTime() const { return IsFinished() ? m_StartTime + ezTime::MakeFromSeconds(m_fDurationInSeconds) : ezTime::MakeZero(); }

    ezUuid m_AssetGuid;
    ezTime m_StartTime;
    float m_fDurationInSeconds = -1;
    ezUInt16 m_uiResultIndex = -1; //< We only store failed results to safe space. Index points to m_FailedTransforms
    ezAssetInfo::TransformState m_TransformState = ezAssetInfo::Unknown;
  };

  class HistoryState : public ezRefCounted
  {
  public:
    void SetMaxProcessors(ezUInt32 uiCount);
    void ClearHistory();
    ezTime GetLatestTaskTime() const;
    void OnProgressEvent(const ezAssetProcessorProgressEvent& e);
    void OnProcessorEvent(const ezAssetProcessorEvent& e);
    const ProcessorTask* FindTaskAtTime(ezUInt32 uiProcessorID, double fPointInTimeSec) const;

  public:
    mutable ezMutex m_HistoryMutex;
    ezQtAssetProcessorProgressWidget* m_pParent = nullptr;
    // Rendering the current time is a bit cumbersome so we instead subtract an offset to make the graph start at zero seconds. If this value is false, the next incoming asset transform will set m_CurrentOffset.
    bool m_bCurrentOffsetValid = false;
    // To compact various runs of asset processing in the timeline, this value is subtracted from the actual start / end time of each asset transform.
    ezTime m_CurrentOffset;
    // At which points in time the timeline got compacted. Just used for rendering a pattern to indicate that the timeline was cut off at these points.
    ezDynamicArray<ezTime> m_SkipOffsets;
    ezDynamicArray<ezDynamicArray<ProcessorTask>> m_ProcessorHistory; // [processorId][taskIndex]
    ezDeque<ezTransformStatus> m_FailedTransforms;
    ezDynamicArray<ezEditorProcessorState> m_ProcessStates;
  };

  static constexpr int s_iRowHeight = 30;
  static constexpr int s_iRowSpacing = 5;
  static constexpr int s_iLeftMargin = 90;
  static constexpr int s_iIndicatorSize = 10;
  static constexpr int s_iTopMargin = 0;

private Q_SLOTS:
  void OnUpdateTimer();
  void OnHistoryChanged();
  void OnProcessorStateChanged();
  void OnProcessStateChanged(ezUInt8 uiProcessID);

private:
  QPoint MapFromScene(const QPointF& pos) const;
  QPointF MapToScene(const QPoint& pos) const;
  QRectF ComputeViewportSceneRect() const;
  void ClampZoomPan();
  void UpdateGridBarConfig() const;

  const ezString& GetAssetPath(const ezUuid& assetGuid) const;
  const ProcessorTask* FindTaskAtPosition(const QPoint& pos, ezUInt32& out_uiProcessorID) const;
  void ShowTooltip(QMouseEvent* e);

  void DrawTimeline(QPainter& painter) const;
  void DrawProcessorRow(QPainter& painter, ezUInt32 uiProcessorID, int y) const;
  void DrawProcessorTask(QPainter& painter, const ProcessorTask& task, QRect rect) const;

private:
  ezEventSubscriptionID m_ProgressEventsID;
  ezEventSubscriptionID m_ProcessorEventsID;
  ezSharedPtr<HistoryState> m_pHistoryState;
  ezUInt32 m_uiMaxProcessors = 0;

  QTimer* m_pUpdateTimer = nullptr;
  ezQGridBarWidget* m_pGridBar = nullptr;
  QScrollBar* m_pScrollBar = nullptr;

  // Display and interaction settings
  EditState m_EditState = EditState::None;
  ezTime m_TimelineLength = ezTime::MakeFromMinutes(1); // Multiple of 1min, resize when current time exceeds this.
  double m_fSceneTranslationX = 0;                      // Scene horizontal pan offset (in seconds)
  QPointF m_SceneToPixelScale = QPointF(20, 1);
  QPoint m_LastMousePos = {0, 0};

  // Cache for asset names so we don't have to store the name in ProcessorTask and also don't SPAM the ezAssetCurator.
  mutable ezMap<ezUuid, ezString> m_AssetNameCache;
  ezString m_sUnknownAsset = "<DELETED>";
};

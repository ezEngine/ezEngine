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

/// \brief Visual progress widget that displays asset processing timeline similar to IncrediBuild
///
/// Shows a timeline view with Y-axis representing each processor and X-axis representing time.
/// Each asset being processed is displayed as a bar on the timeline.
class EZ_EDITORFRAMEWORK_DLL ezQtAssetProcessorProgressWidget : public QWidget
{
  Q_OBJECT

public:
  explicit ezQtAssetProcessorProgressWidget(QWidget* pParent = nullptr);
  ~ezQtAssetProcessorProgressWidget();

  void SetGridBarWidget(ezQGridBarWidget* pGridBar);

public Q_SLOTS:
  void ClearHistory();

protected:
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  virtual QSize sizeHint() const override;
  virtual QSize minimumSizeHint() const override;

private:
  enum class EditState
  {
    None,
    Panning
  };

  struct ProcessorTask
  {
    static constexpr ezUInt16 SuccessResult = 0xFFFF;
    EZ_ALWAYS_INLINE bool IsFinished() const { return m_fDurationInSeconds != -1; }
    EZ_ALWAYS_INLINE bool Failed() const { return m_uiResultIndex != SuccessResult; }
    EZ_ALWAYS_INLINE ezTime EndTime() const { return IsFinished() ? m_StartTime + ezTime::MakeFromSeconds(m_fDurationInSeconds) : ezTime::MakeZero(); }

    ezUuid m_AssetGuid;
    ezTime m_StartTime;
    float m_fDurationInSeconds = -1;
    ezUInt16 m_uiResultIndex = -1; //< We only store failed results to safe space in m_FailedTransforms
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
    const ProcessorTask* FindTaskAtTime(ezUInt32 uiProcessorID, double pointInTime) const;

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
  static constexpr int s_iLeftMargin = 80;
  static constexpr int s_iIndicatorSize = 10;
  static constexpr int s_iTopMargin = 0;

private Q_SLOTS:
  void OnUpdateTimer();
  void OnHistoryChanged();
  void OnProcessStateChanged(ezUInt8 uiProcessID);

private:
  QPoint MapFromScene(const QPointF& pos) const;
  QPointF MapToScene(const QPoint& pos) const;
  void ShowTooltip(QMouseEvent* e);
  const ezString& GetAssetName(const ezUuid& assetGuid) const;
  void DrawTimeline(QPainter& painter) const;
  void DrawProcessorRow(QPainter& painter, ezUInt32 uiProcessorID, int y) const;
  void DrawProcessorTask(QPainter& painter, const ProcessorTask& task, QRect rect) const;
  void ClampZoomPan();
  void UpdateGridBarConfig() const;
  QRectF ComputeViewportSceneRect() const;
  const ProcessorTask* FindTaskAtPosition(const QPoint& pos, ezUInt32& out_uiProcessorID) const;

private:
  ezEventSubscriptionID m_ProgressEventsID;
  ezEventSubscriptionID m_ProcessorEventsID;
  ezSharedPtr<HistoryState> m_pHistoryState;
  ezUInt32 m_uiMaxProcessors = 0;

  QTimer* m_pUpdateTimer = nullptr;
  ezQGridBarWidget* m_pGridBar = nullptr;

  // Display and interaction settings
  EditState m_EditState = EditState::None;
  ezTime m_TimelineLength = ezTime::MakeZero(); // Multiple of 1min, resize when current time exceeds this.
  double m_fSceneTranslationX = -100.0;         // Scene horizontal pan offset (in seconds)
  QPointF m_SceneToPixelScale = QPointF(20, 1);
  QPoint m_LastMousePos = {0, 0};

  // Cache for asset names so we don't have to store the name in ProcessorTask and also don't SPAM the ezAssetCurator.
  mutable ezMap<ezUuid, ezString> m_AssetNameCache;
  ezString m_sUnknownAsset = "<DELETED>";
};

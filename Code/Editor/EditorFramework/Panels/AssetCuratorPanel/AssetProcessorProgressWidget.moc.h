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

  QPoint MapFromScene(const QPointF& pos) const;
  QPointF MapToScene(const QPoint& pos) const;

public Q_SLOTS:
  void ClearHistory();

protected:
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  void ShowTooltip(QMouseEvent* e);
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  virtual QSize sizeHint() const override;
  virtual QSize minimumSizeHint() const override;

private Q_SLOTS:
  void OnUpdateTimer();
  void OnHistoryChanged();
  void OnProcessStateChanged(ezUInt8 uiProcessID);

private:
  enum class EditState
  {
    None,
    Panning
  };

  struct ProcessorTask
  {
    ezUuid m_AssetGuid;
    ezString m_sAssetPath;
    ezTime m_StartTime;
    ezTime m_EndTime;
    ezTransformStatus m_Result;
    ezAssetInfo::TransformState m_TransformState = ezAssetInfo::Unknown;
    bool m_bFinished = false;
  };

  class HistoryState : public ezRefCounted
  {
  public:
    void SetMaxProcessors(ezUInt32 uiCount);
    void ClearHistory();
    ezTime GetLatestTaskTime() const;
    void OnProgressEvent(const ezAssetProcessorProgressEvent& e);
    void OnProcessorEvent(const ezAssetProcessorEvent& e);

  public:
    mutable ezMutex m_HistoryMutex;
    ezQtAssetProcessorProgressWidget* m_pParent = nullptr;
    // Rendering the current time is a bit cumbersome so we instead subtract an offset to make the graph start at zero seconds.
    bool m_bCurrentOffsetValid = false;
    ezTime m_CurrentOffset;
    ezDynamicArray<ezTime> m_SkipOffsets;
    ezDynamicArray<ezDynamicArray<ProcessorTask>> m_ProcessorHistory; // [processor][task history]
    ezDynamicArray<ezEditorProcessorState> m_ProcessStates;
  };

  static constexpr int s_iRowHeight = 30;
  static constexpr int s_iRowSpacing = 5;
  static constexpr int s_iLeftMargin = 80;
  static constexpr int s_iIndicatorSize = 10;
  static constexpr int s_iTopMargin = 0;

private:
  void DrawTimeline(QPainter& painter);
  void DrawProcessorRow(QPainter& painter, ezUInt32 uiProcessorID, int y, int rowHeight);
  void ClampZoomPan();
  void UpdateGridBarConfig() const;
  QRectF ComputeViewportSceneRect() const;

  const ProcessorTask* FindTaskAtPosition(const QPoint& pos, ezUInt32& out_uiProcessorID) const;

private:
  ezEventSubscriptionID m_ProgressEventsID;
  ezEventSubscriptionID m_ProcessorEventsID;
  ezSharedPtr<HistoryState> m_pHistoryState;
  ezUInt32 m_uiMaxProcessors = 0;

  // TODO: Find first element to render in logn
  // TODO: Only update when active processing
  QTimer* m_pUpdateTimer = nullptr;
  ezQGridBarWidget* m_pGridBar = nullptr;

  // Display and interaction settings
  EditState m_EditState = EditState::None;
  ezTime m_TimelineLength = ezTime::MakeZero(); // Multiple of 1min, resize when current time exceeds this.
  double m_fSceneTranslationX = -100.0;         // Scene horizontal pan offset (in seconds)
  QPointF m_SceneToPixelScale = QPointF(20, 1);
  QPoint m_LastMousePos = {0, 0};
};

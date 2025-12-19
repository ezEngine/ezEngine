#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
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

  void SetMaxProcessors(ezUInt32 uiCount);
  void SetGridBarWidget(ezQGridBarWidget* pGridBar);

  QPoint MapFromScene(const QPointF& pos) const;
  QPointF MapToScene(const QPoint& pos) const;

protected:
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  virtual QSize sizeHint() const override;
  virtual QSize minimumSizeHint() const override;

private Q_SLOTS:
  void OnUpdateTimer();
  void OnClearHistory();

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
    bool m_bFinished = false;
    bool m_bSuccess = false;
  };

  void OnProgressEvent(const ezAssetProcessorProgressEvent& e);
  void OnProcessorEvent(const ezAssetProcessorEvent& e);
  void DrawTimeline(QPainter& painter);
  void DrawProcessorRow(QPainter& painter, ezUInt32 uiProcessorID, int y, int rowHeight);
  QString GetShortAssetName(const ezString& sPath) const;
  void ClampZoomPan();
  void UpdateGridBarConfig();
  QRectF ComputeViewportSceneRect() const;
  ezTime GetLatestTaskTime() const;

  ezUInt32 m_uiMaxProcessors = 0;
  ezDynamicArray<ezDynamicArray<ProcessorTask>> m_ProcessorHistory; // [processor][task history]
  QTimer* m_pUpdateTimer = nullptr;
  QPushButton* m_pClearButton = nullptr;
  ezQGridBarWidget* m_pGridBar = nullptr;
  
  // Display and interaction settings
  EditState m_State = EditState::None;
  double m_fSceneTranslationX = 0.0; // Scene horizontal pan offset (in seconds)
  QPointF m_SceneToPixelScale = QPointF(1, 1);
  QPoint m_LastMousePos;
  
  static constexpr int s_iRowHeight = 30;
  static constexpr int s_iRowSpacing = 5;
  static constexpr int s_iLeftMargin = 80;
  static constexpr int s_iTopMargin = 30;
};

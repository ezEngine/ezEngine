#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>
#include <QWidget>

struct ezAssetProcessorProgressEvent;

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

protected:
  virtual void paintEvent(QPaintEvent* event) override;
  virtual QSize sizeHint() const override;
  virtual QSize minimumSizeHint() const override;

private Q_SLOTS:
  void OnUpdateTimer();

private:
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
  void DrawTimeline(QPainter& painter);
  void DrawProcessorRow(QPainter& painter, ezUInt32 uiProcessorID, int y, int rowHeight, ezTime currentTime);
  QString GetShortAssetName(const ezString& sPath) const;

  ezUInt32 m_uiMaxProcessors = 0;
  ezDynamicArray<ezDynamicArray<ProcessorTask>> m_ProcessorHistory; // [processor][task history]
  QTimer* m_pUpdateTimer = nullptr;
  
  // Display settings
  ezTime m_TimeWindowDuration = ezTime::MakeFromSeconds(60.0); // Show last 60 seconds
  static constexpr int s_iRowHeight = 30;
  static constexpr int s_iRowSpacing = 5;
  static constexpr int s_iLeftMargin = 80;
  static constexpr int s_iTopMargin = 30;
};

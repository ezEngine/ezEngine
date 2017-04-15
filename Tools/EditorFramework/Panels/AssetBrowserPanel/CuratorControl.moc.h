#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/Plugin.h>
#include <QWidget>

struct ezAssetCuratorEvent;
struct ezToolsProjectEvent;
struct ezAssetProcessorEvent;
class QToolButton;

/// \brief
class EZ_EDITORFRAMEWORK_DLL ezQtCuratorControl : public QWidget
{
  Q_OBJECT
public:
  explicit ezQtCuratorControl(QWidget* pParent);
  ~ezQtCuratorControl();

protected:
  virtual void paintEvent(QPaintEvent* e) override;

private slots:
  void SlotUpdateTransformStats();
  void UpdateBackgroundProcessState();
  void BackgroundProcessClicked(bool checked);

private:
  void ScheduleUpdateTransformStats();
  void AssetCuratorEvents(const ezAssetCuratorEvent& e);
  void AssetProcessorEvents(const ezAssetProcessorEvent& e);
  void ProjectEvents(const ezToolsProjectEvent& e);

  bool m_bScheduled;
  QToolButton* m_pBackgroundProcess;
};

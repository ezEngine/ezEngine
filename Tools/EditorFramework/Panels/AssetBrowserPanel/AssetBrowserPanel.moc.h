#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/Plugin.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <Tools/EditorFramework/ui_AssetBrowserPanel.h>

class QStatusBar;
class QLabel;
class QProgressBar;
struct ezToolsProjectEvent;

/// \brief The application wide panel that shows and asset browser.
class EZ_EDITORFRAMEWORK_DLL ezQtAssetBrowserPanel : public ezQtApplicationPanel, public Ui_AssetBrowserPanel
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtAssetBrowserPanel);

public:
  ezQtAssetBrowserPanel();
  ~ezQtAssetBrowserPanel();

private slots:
  void SlotAssetChosen(QString sAssetGuid, QString sAssetPathRelative, QString sAssetPathAbsolute);
  void SlotUpdateTransformStats();

private:
  void ScheduleUpdateTransformStats();
  void AssetCuratorEvents(const ezAssetCuratorEvent& e);
  void ProjectEvents(const ezToolsProjectEvent& e);

  bool m_bScheduled;
  QStatusBar* m_pStatusBar;
  //QLabel* m_pStatusText;
  QProgressBar* m_pProgress;
};
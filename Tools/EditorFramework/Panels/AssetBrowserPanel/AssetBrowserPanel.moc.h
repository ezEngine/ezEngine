#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/Plugin.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <Tools/EditorFramework/ui_AssetBrowserPanel.h>

class QStatusBar;
class QLabel;
struct ezToolsProjectEvent;
class ezQtCuratorControl;

/// \brief The application wide panel that shows and asset browser.
class EZ_EDITORFRAMEWORK_DLL ezQtAssetBrowserPanel : public ezQtApplicationPanel, public Ui_AssetBrowserPanel
{
  Q_OBJECT

  EZ_DECLARE_SINGLETON(ezQtAssetBrowserPanel);

public:
  ezQtAssetBrowserPanel();
  ~ezQtAssetBrowserPanel();

private slots:
  void SlotAssetChosen(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute);

private:
  void AssetCuratorEvents(const ezAssetCuratorEvent& e);
  void ProjectEvents(const ezToolsProjectEvent& e);

  QStatusBar* m_pStatusBar;
  ezQtCuratorControl* m_pCuratorControl;
};
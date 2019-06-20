#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <Editor/EditorFramework/ui_AssetBrowserPanel.h>

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

  const ezUuid& GetLastSelectedAsset() const { return m_LastSelected; }

private Q_SLOTS:
  void SlotAssetChosen(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute);
  void SlotAssetSelected(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute);
  void SlotAssetCleared();

private:
  void AssetCuratorEvents(const ezAssetCuratorEvent& e);
  void ProjectEvents(const ezToolsProjectEvent& e);

  ezUuid m_LastSelected;
  QStatusBar* m_pStatusBar;
  ezQtCuratorControl* m_pCuratorControl;
};

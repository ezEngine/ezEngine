#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/Plugin.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <Tools/EditorFramework/ui_AssetBrowserPanel.h>

/// \brief The application wide panel that shows and asset browser.
class EZ_EDITORFRAMEWORK_DLL ezQtAssetBrowserPanel : public ezQtApplicationPanel, public Ui_AssetBrowserPanel
{
  Q_OBJECT

public:
  ezQtAssetBrowserPanel();
  ~ezQtAssetBrowserPanel();

  static ezQtAssetBrowserPanel* GetInstance() { return s_pInstance; }

private slots:
  void SlotAssetChosen(QString sAssetGuid, QString sAssetPathRelative, QString sAssetPathAbsolute);

private:
  static ezQtAssetBrowserPanel* s_pInstance;
};
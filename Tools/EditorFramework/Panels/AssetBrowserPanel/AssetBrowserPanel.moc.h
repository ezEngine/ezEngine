#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/Plugin.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <Tools/EditorFramework/ui_AssetBrowserPanel.h>

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

};
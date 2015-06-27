#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/Plugin.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <Tools/EditorFramework/ui_AssetBrowserPanel.h>

class EZ_EDITORFRAMEWORK_DLL ezAssetBrowserPanel : public ezApplicationPanel, public Ui_AssetBrowserPanel
{
  Q_OBJECT

public:
  ezAssetBrowserPanel();
  ~ezAssetBrowserPanel();

  static ezAssetBrowserPanel* GetInstance() { return s_pInstance; }

private slots:
  void SlotAssetChosen(QString sAssetGuid, QString sAssetPathRelative, QString sAssetPathAbsolute);

private:
  static ezAssetBrowserPanel* s_pInstance;
};
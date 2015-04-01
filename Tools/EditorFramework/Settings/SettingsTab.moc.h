#pragma once

#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/GUI/SimplePropertyGridWidget.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <Tools/EditorFramework/ui_SettingsTab.h>
#include <Foundation/Configuration/Plugin.h>

class ezSettingsTab : public ezDocumentWindow, Ui_SettingsTab
{
  Q_OBJECT

public:
  ezSettingsTab();
  ~ezSettingsTab();

  static ezSettingsTab* GetInstance();

  virtual const char* GetGroupName() const override { return "Settings"; }
  
private slots:
  void SlotSettingsChanged();
  void on_ButtonPluginConfig_clicked();
  void SlotComboSettingsDomainIndexChanged(int iIndex);
  void on_ButtonDataDirConfig_clicked();

private:
  virtual bool InternalCanCloseWindow() override;
  virtual void InternalCloseDocumentWindow() override;

  void PluginEventHandler(const ezPlugin::PluginEvent& e);
  void UpdateSettings();
  void ProjectEventHandler(const ezToolsProject::Event& e);
  void DocumentManagerEventHandler(const ezDocumentManagerBase::Event& e);

  ezString m_sSelectedSettingDomain;
  ezMap<ezString, ezVariant> m_Settings;

  ezSimplePropertyGridWidget* m_pSettingsGrid;

};




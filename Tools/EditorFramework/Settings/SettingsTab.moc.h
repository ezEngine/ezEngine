#pragma once

#include <EditorFramework/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/GUI/SimplePropertyGridWidget.moc.h>
#include <EditorFramework/Project/EditorProject.h>
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
  void SlotButtonPluginConfig();
  void SlotComboSettingsDomainIndexChanged(int iIndex);

private:
  virtual bool InternalCanClose();
  virtual void InternalCloseDocument();

  void PluginEventHandler(const ezPlugin::PluginEvent& e);
  void UpdateSettings();
  void ProjectEventHandler(const ezEditorProject::Event& e);
  void DocumentManagerEventHandler(const ezDocumentManagerBase::Event& e);

  ezString m_sSelectedSettingDomain;
  ezMap<ezString, ezVariant> m_Settings;

  ezSimplePropertyGridWidget* m_pSettingsGrid;

};




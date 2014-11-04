#pragma once

#include <EditorFramework/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/GUI/SimplePropertyGridWidget.moc.h>
#include <Tools/EditorFramework/ui_SettingsTab.h>

//class ezSettingsTabCenter : public QWidget, Ui_SettingsTab
//{
//  Q_OBJECT
//
//
//};

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

private:
  virtual bool InternalCanClose();
  virtual void InternalCloseDocument();

  ezMap<ezString, ezVariant> m_Settings;

  ezSimplePropertyGridWidget* m_pSettingsGrid;

};




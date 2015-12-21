#pragma once

#include <EditorFramework/Plugin.h>
#include <QDialog>
#include <Tools/EditorFramework/ui_SettingsDlg.h>
#include <Foundation/Strings/String.h>
#include <EditorFramework/GUI/SimplePropertyGridWidget.moc.h>

class EZ_EDITORFRAMEWORK_DLL SettingsDlg : public QDialog, public Ui_SettingsDlg
{
public:
  Q_OBJECT

public:
  SettingsDlg(QWidget* parent);

private slots:
  void SlotSettingsChanged();
  void SlotComboSettingsDomainIndexChanged(int iIndex);

private:
  void UpdateSettings();

  ezString m_sSelectedSettingDomain;
  ezMap<ezString, ezVariant> m_Settings;

  ezQtSimplePropertyGridWidget* m_pSettingsGrid;

};



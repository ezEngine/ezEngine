#pragma once

#include <EditorFramework/Plugin.h>
#include <QDialog>
#include <Tools/EditorFramework/ui_PreferencesDlg.h>
#include <Foundation/Strings/String.h>

class EZ_EDITORFRAMEWORK_DLL PreferencesDlg : public QDialog, public Ui_PreferencesDlg
{
public:
  Q_OBJECT

public:
  PreferencesDlg(QWidget* parent);

private slots:
  void SlotSettingsChanged();
  void SlotComboSettingsDomainIndexChanged(int iIndex);

private:
  void UpdateSettings();

  //ezString m_sSelectedSettingDomain;
  //ezMap<ezString, ezVariant> m_Settings;

  //ezQtSimplePropertyGridWidget* m_pSettingsGrid;

};



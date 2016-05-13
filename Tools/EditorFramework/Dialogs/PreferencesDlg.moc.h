#pragma once

#include <EditorFramework/Plugin.h>
#include <QDialog>
#include <Tools/EditorFramework/ui_PreferencesDlg.h>
#include <Foundation/Strings/String.h>

class ezPreferencesDocument;
class ezPreferences;

class EZ_EDITORFRAMEWORK_DLL PreferencesDlg : public QDialog, public Ui_PreferencesDlg
{
public:
  Q_OBJECT

public:
  PreferencesDlg(QWidget* parent);
  ~PreferencesDlg();

  ezUuid NativeToObject(ezPreferences* pPreferences);
  void ObjectToNative(ezUuid objectGuid);


private slots:
  void SlotComboSettingsDomainIndexChanged(int iIndex);

private:
  void UpdateSettings();
  void PropertyChangedEventHandler(const ezDocumentObjectPropertyEvent& e);

  ezString m_sSelectedSettingDomain;
  //ezMap<ezString, ezVariant> m_Settings;
  ezPreferencesDocument* m_pDocument;
};



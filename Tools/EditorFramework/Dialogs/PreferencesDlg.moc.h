#pragma once

#include <EditorFramework/Plugin.h>
#include <QDialog>
#include <Tools/EditorFramework/ui_PreferencesDlg.h>
#include <Foundation/Strings/String.h>

class ezPreferencesDocument;
class ezPreferences;
class ezQtDocumentTreeView;

class EZ_EDITORFRAMEWORK_DLL ezQtPreferencesDlg : public QDialog, public Ui_ezQtPreferencesDlg
{
public:
  Q_OBJECT

public:
  ezQtPreferencesDlg(QWidget* parent);
  ~ezQtPreferencesDlg();

  ezUuid NativeToObject(ezPreferences* pPreferences);
  void ObjectToNative(ezUuid objectGuid, const ezDocument* pPrefDocument);


private slots:
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked() { reject(); }

private:
  void RegisterAllPreferenceTypes();
  void AllPreferencesToObject();
  void PropertyChangedEventHandler(const ezDocumentObjectPropertyEvent& e);
  void ApplyAllChanges();

  ezPreferencesDocument* m_pDocument;
  ezMap<ezUuid, const ezDocument*> m_DocumentBinding;
};



#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Tools/EditorFramework/ui_AssetProfilesDlg.h>

#include <QDialog>

class ezAssetProfilesDocument;
class ezAssetProfile;
class ezQtDocumentTreeView;
class ezDocument;
struct ezDocumentObjectPropertyEvent;

class EZ_EDITORFRAMEWORK_DLL ezQtAssetProfilesDlg : public QDialog, public Ui_ezQtAssetProfilesDlg
{
public:
  Q_OBJECT

public:
  ezQtAssetProfilesDlg(QWidget* parent);
  ~ezQtAssetProfilesDlg();

  ezUuid NativeToObject(ezAssetProfile* pConfig);
  void ObjectToNative(ezUuid objectGuid, ezAssetProfile* pConfig);

  ezUInt32 m_uiActiveConfig = 0;

private slots:
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void OnItemDoubleClicked(QModelIndex idx);

private:
  void AllAssetProfilesToObject();
  void PropertyChangedEventHandler(const ezDocumentObjectPropertyEvent& e);
  void ApplyAllChanges();

  ezAssetProfilesDocument* m_pDocument;
  ezMap<ezUuid, ezAssetProfile*> m_ConfigBinding;
};

#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Tools/EditorFramework/ui_AssetConfigsDlg.h>

#include <QDialog>

class ezAssetConfigsDocument;
class ezAssetPlatformConfig;
class ezQtDocumentTreeView;
class ezDocument;
struct ezDocumentObjectPropertyEvent;

class EZ_EDITORFRAMEWORK_DLL ezQtAssetConfigsDlg : public QDialog, public Ui_ezQtAssetConfigsDlg
{
public:
  Q_OBJECT

public:
  ezQtAssetConfigsDlg(QWidget* parent);
  ~ezQtAssetConfigsDlg();

  ezUuid NativeToObject(ezAssetPlatformConfig* pConfig);
  void ObjectToNative(ezUuid objectGuid, ezAssetPlatformConfig* pConfig);


private slots:
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked() { reject(); }

private:
  void AllAssetConfigsToObject();
  void PropertyChangedEventHandler(const ezDocumentObjectPropertyEvent& e);
  void ApplyAllChanges();

  ezAssetConfigsDocument* m_pDocument;
  ezMap<ezUuid, ezAssetPlatformConfig*> m_ConfigBinding;
};

#pragma once

#include <EditorPluginFmod/Plugin.h>
#include <QDialog>
#include <Tools/EditorPluginFmod/ui_FmodProjectSettingsDlg.h>
#include <FmodPlugin/FmodSingleton.h>

class ezQtFmodProjectSettingsDlg : public QDialog, public Ui_FmodProjectSettingsDlg
{
public:
  Q_OBJECT

public:
  ezQtFmodProjectSettingsDlg(QWidget* parent);

private slots:
  void on_ButtonBox_clicked(QAbstractButton* pButton);
  void on_ListPlatforms_itemSelectionChanged();
  void on_ButtonAdd_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonMB_clicked();

private:
  ezResult Save();
  void Load();
  void SetCurrentPlatform(const char* szPlatform);
  void StoreCurrentPlatform();

  ezString m_sCurrentPlatform;
  ezFmodAssetProfiles m_ConfigsOld;
  ezFmodAssetProfiles m_Configs;
};

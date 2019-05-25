#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <QDialog>
#include <Tools/EditorFramework/ui_DataDirsDlg.h>
#include <Foundation/Application/Config/FileSystemConfig.h>

class EZ_EDITORFRAMEWORK_DLL ezQtDataDirsDlg : public QDialog, public Ui_ezQtDataDirsDlg
{
public:
  Q_OBJECT

public:
  ezQtDataDirsDlg(QWidget* parent);

private Q_SLOTS:
  void on_ButtonOK_clicked();
  void on_ButtonCancel_clicked();
  void on_ButtonUp_clicked();
  void on_ButtonDown_clicked();
  void on_ButtonAdd_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonOpenFolder_clicked();
  void on_ListDataDirs_itemSelectionChanged();
  void on_ListDataDirs_itemDoubleClicked(QListWidgetItem* pItem);

private:
  void FillList();

  ezInt32 m_iSelection;
  ezApplicationFileSystemConfig m_Config;
};



#pragma once

#include <EditorFramework/Plugin.h>
#include <QDialog>
#include <Tools/EditorFramework/ui_DataDirsDlg.h>
#include <Core/Application/Config/FileSystemConfig.h>

class EZ_EDITORFRAMEWORK_DLL DataDirsDlg : public QDialog, public Ui_DataDirsDlg
{
public:
  Q_OBJECT

public:
  DataDirsDlg(QWidget* parent);

private slots:
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



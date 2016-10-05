#pragma once

#include <EditorFramework/Plugin.h>
#include <Tools/EditorFramework/ui_TagsDlg.h>
#include <Foundation/Strings/String.h>
#include <QDialog>
#include <Foundation/Containers/Map.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

class EZ_EDITORFRAMEWORK_DLL ezQtTagsDlg : public QDialog, public Ui_ezQtTagsDlg
{
public:
  Q_OBJECT

public:
  ezQtTagsDlg(QWidget* parent);

  private slots:
  void on_ButtonNewCategory_clicked();
  void on_ButtonNewTag_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void on_ButtonReset_clicked();
  void on_TreeTags_itemSelectionChanged();

private:
  void LoadTags();
  void SaveTags();
  void FillList();
  void GetTagsFromList();

  QTreeWidgetItem* CreateTagItem(QTreeWidgetItem* pParentItem, const QString& tag);

  ezHybridArray<ezToolsTag, 32> m_Tags;
  ezMap<ezString, QTreeWidgetItem*> m_CategoryToItem;

};



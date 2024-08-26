#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_EditDynamicEnumsDlg.h>
#include <Foundation/Strings/String.h>
#include <QDialog>

class ezDynamicStringEnum;

class EZ_EDITORFRAMEWORK_DLL ezQtEditDynamicEnumsDlg : public QDialog, public Ui_ezQtEditDynamicEnumsDlg
{
public:
  Q_OBJECT

public:
  ezQtEditDynamicEnumsDlg(ezDynamicStringEnum* pEnum, QWidget* pParent);

  ezInt32 GetSelectedItem() const { return m_iSelectedItem; }

private Q_SLOTS:
  void on_ButtonAdd_clicked();
  void on_ButtonRemove_clicked();
  void on_Buttons_clicked(QAbstractButton* button);
  void on_EnumValues_itemDoubleClicked(QListWidgetItem* item);

private:
  void FillList();
  bool EditItem(ezString& item);

  bool m_bModified = false;
  ezDynamicStringEnum* m_pEnum = nullptr;
  ezDynamicArray<ezString> m_Values;
  ezInt32 m_iSelectedItem = -1;
};

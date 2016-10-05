#pragma once

#include <EditorFramework/Plugin.h>
#include <QDialog>
#include <Tools/EditorFramework/ui_InputConfigDlg.h>
#include <GameFoundation/GameApplication/InputConfig.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>

class QTreeWidgetItem;

class EZ_EDITORFRAMEWORK_DLL ezQtInputConfigDlg : public QDialog, public Ui_InputConfigDialog
{
public:
  Q_OBJECT

public:
  ezQtInputConfigDlg(QWidget* parent);

private slots:
  void on_ButtonNewInputSet_clicked();
  void on_ButtonNewAction_clicked();
  void on_ButtonRemove_clicked();
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void on_ButtonReset_clicked();
  void on_TreeActions_itemSelectionChanged();

private:
  void LoadActions();
  void SaveActions();
  void FillList();
  void GetActionsFromList();

  QTreeWidgetItem* CreateActionItem(QTreeWidgetItem* pParentItem, const ezGameAppInputConfig& action);

  ezMap<ezString, QTreeWidgetItem*> m_InputSetToItem;
  ezHybridArray<ezGameAppInputConfig, 32> m_Actions;
  ezDynamicArray<ezString> m_AllInputSlots;
};



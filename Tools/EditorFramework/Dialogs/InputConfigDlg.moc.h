#pragma once

#include <EditorFramework/Plugin.h>
#include <QDialog>
#include <Tools/EditorFramework/ui_InputConfigDlg.h>
#include <GameFoundation/GameApplication/InputConfig.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>

class QTreeWidgetItem;

class EZ_EDITORFRAMEWORK_DLL InputConfigDlg : public QDialog, public Ui_InputConfigDialog
{
public:
  Q_OBJECT

public:
  InputConfigDlg(QWidget* parent);

private slots:
	void on_ButtonNewInputSet_clicked();
	void on_ButtonNewAction_clicked();
	void on_ButtonRemove_clicked();
	void on_ButtonBox_clicked( QAbstractButton* pButton );
	void on_TreeActions_itemSelectionChanged();

private:
  void LoadActions();
  void FillList();

  QTreeWidgetItem* CreateActionItem( QTreeWidgetItem* pParentItem, const ezGameAppInputConfig& action );

  ezMap<ezString, QTreeWidgetItem*> m_InputSetToItem;
  ezHybridArray<ezGameAppInputConfig, 32> m_Actions;
  ezSet<ezString> m_AllInputSlots;
};



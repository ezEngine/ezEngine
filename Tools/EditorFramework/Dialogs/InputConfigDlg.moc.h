#pragma once

#include <EditorFramework/Plugin.h>
#include <QDialog>
#include <Tools/EditorFramework/ui_InputConfigDlg.h>
#include <GameFoundation/GameApplication/InputConfig.h>
#include <Foundation/Containers/Map.h>

class QTreeWidgetItem;

class EZ_EDITORFRAMEWORK_DLL InputConfigDlg : public QDialog, public Ui_InputConfigDialog
{
public:
  Q_OBJECT

public:
  InputConfigDlg(QWidget* parent);

private slots:

private:
  void LoadActions();
  void FillList();

  ezMap<ezString, QTreeWidgetItem*> m_InputSetToItem;
  ezHybridArray<ezGameAppInputConfig, 32> m_Actions;
};



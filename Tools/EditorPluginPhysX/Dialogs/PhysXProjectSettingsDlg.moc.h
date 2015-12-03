#pragma once

#include <EditorPluginPhysX/Plugin.h>
#include <QDialog>
#include <Tools/EditorPluginPhysX/ui_PhysXProjectSettingsDlg.h>
#include <GameUtils/CollisionFilter/CollisionFilter.h>

class ezPhysxProjectSettingsDlg : public QDialog, public Ui_PhysXProjectSettingsDlg
{
public:
  Q_OBJECT

public:
  ezPhysxProjectSettingsDlg(QWidget* parent);

private slots:
  void onCheckBoxClicked(bool checked);
  void on_DefaultButtons_clicked(QAbstractButton* pButton);
  void on_ButtonAddLayer_clicked();
  void on_ButtonRemoveLayer_clicked();
  void on_ButtonRenameLayer_clicked();
  void on_FilterTable_itemSelectionChanged();

private:
  void SetupTable();
  ezResult Save();
  ezResult Load();

  ezUInt32 m_IndexRemap[32];
  ezCollisionFilterConfig m_Config;
  ezCollisionFilterConfig m_ConfigReset;
};



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
  ezPhysxProjectSettingsDlg(ezCollisionFilterConfig* pFilterCfg, QWidget* parent);

private slots:
  void onCheckBoxClicked(bool checked);
  void on_DefaultButtons_clicked(QAbstractButton* pButton);
  void on_ButtonAddLayer_clicked();
  void on_ButtonRemoveLayer_clicked();

private:
  void SetupTable();

  ezCollisionFilterConfig m_Config;
  ezCollisionFilterConfig* m_pFilterCfg;
};



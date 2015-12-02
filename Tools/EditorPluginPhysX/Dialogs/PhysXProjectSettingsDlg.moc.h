#pragma once

#include <EditorPluginPhysX/Plugin.h>
#include <QDialog>
#include <Tools/EditorPluginPhysX/ui_PhysXProjectSettingsDlg.h>

class ezPhysxProjectSettingsDlg : public QDialog, public Ui_PhysXProjectSettingsDlg
{
public:
  Q_OBJECT

public:
  ezPhysxProjectSettingsDlg(QWidget* parent);

private slots:

private:

};



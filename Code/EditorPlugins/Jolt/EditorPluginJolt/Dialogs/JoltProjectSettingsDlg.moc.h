#pragma once

#include <EditorPluginJolt/EditorPluginJoltDLL.h>
#include <EditorPluginJolt/ui_JoltProjectSettingsDlg.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <QDialog>

class ezQtJoltProjectSettingsDlg : public QDialog, public Ui_JoltProjectSettingsDlg
{
public:
  Q_OBJECT

public:
  ezQtJoltProjectSettingsDlg(QWidget* parent);

private Q_SLOTS:
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


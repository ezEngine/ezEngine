#pragma once

#include <EditorPluginJolt/EditorPluginJoltDLL.h>
#include <EditorPluginJolt/ui_JoltProjectSettingsDlg.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <GameEngine/Physics/WeightCategory.h>
#include <QDialog>

class ezQtJoltProjectSettingsDlg : public QDialog, public Ui_JoltProjectSettingsDlg
{
public:
  Q_OBJECT

public:
  ezQtJoltProjectSettingsDlg(const ezVariant& startup = {}, QWidget* pParent = nullptr);

  static void EnsureConfigFileExists();

private Q_SLOTS:
  void onCheckBoxClicked(bool checked);
  void onWeightChanged(double value);
  void onWeightDescChanged(const QString& txt);
  void on_DefaultButtons_clicked(QAbstractButton* pButton);
  void on_ButtonAddLayer_clicked();
  void on_ButtonRemoveLayer_clicked();
  void on_ButtonRenameLayer_clicked();
  void on_FilterTable_itemSelectionChanged();

  void on_ButtonAddCategory_clicked();
  void on_ButtonRemoveCategory_clicked();
  void on_ButtonRenameCategory_clicked();

private:
  static void EnsureFilterConfigFileExists();

  void SetupFilterTable();
  void SetupWeightTable();
  ezResult Save();
  ezResult Load();

  ezUInt32 m_IndexRemap[32];
  ezHybridArray<ezHashedString, 32> m_RowToWeight;
  ezCollisionFilterConfig m_Config;
  ezCollisionFilterConfig m_ConfigReset;
  ezWeightCategoryConfig m_WeightConfig;
  ezWeightCategoryConfig m_WeightConfigReset;
  static void EnsureWeightsConfigFileExists();
};

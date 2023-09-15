#pragma once

#include <AiPlugin/Navigation/NavMesh.h>
#include <EditorPluginAi/EditorPluginAiDLL.h>
#include <EditorPluginAi/ui_AiProjectSettingsDlg.h>
#include <QDialog>

class ezQtAiProjectSettingsDlg : public QDialog, public Ui_AiProjectSettingsDlg
{
public:
  Q_OBJECT

public:
  ezQtAiProjectSettingsDlg(QWidget* pParent);

  static void EnsureConfigFileExists();

private Q_SLOTS:
  void on_DefaultButtons_clicked(QAbstractButton* pButton);

  void on_AddPathCfg_clicked();
  void on_RemovePathCfg_clicked();
  void on_SelectedPathCfg_currentIndexChanged(int index);

  void on_AddMeshCfg_clicked();
  void on_RemoveMeshCfg_clicked();
  void on_SelectedMeshCfg_currentIndexChanged(int index);


private:
  void ResetState();
  void SaveState();
  void UpdateGroundTypeTable();
  void RetrieveGroundTypeTable();

  void FillPathSearchTypeComboBox();
  void ApplyPathConfig(int index);
  void RetrievePathConfig(int index);

  void FillNavmeshTypeComboBox();
  void ApplyNavmeshConfig(int index);
  void RetrieveNavmeshConfig(int index);

  int m_iSelectedPathSearchConfig = -1;
  int m_iSelectedNavmeshConfig = -1;
  ezAiNavigationConfig m_Config;
};

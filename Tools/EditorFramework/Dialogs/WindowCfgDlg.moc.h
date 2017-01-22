#pragma once

#include <EditorFramework/Plugin.h>
#include <QDialog>
#include <Tools/EditorFramework/ui_WindowCfgDlg.h>
#include <Core/Application/Config/FileSystemConfig.h>
#include <System/Window/Window.h>

class EZ_EDITORFRAMEWORK_DLL ezQtWindowCfgDlg : public QDialog, public Ui_ezQtWindowCfgDlg
{
public:
  Q_OBJECT

public:
  ezQtWindowCfgDlg(QWidget* parent);

private slots:
  void on_m_ButtonBox_clicked(QAbstractButton * button);
  void on_m_ComboWnd_currentIndexChanged(int index);
  void on_m_CheckOverrideDefault_stateChanged(int state);

private:
  void FillUI(const ezWindowCreationDesc& desc);
  void GrabUI(ezWindowCreationDesc& desc);
  void UpdateUI();
  void LoadDescs();
  void SaveDescs();

  ezUInt8 m_uiCurDesc = 0;
  ezWindowCreationDesc m_Descs[2];
  bool m_bOverrideProjectDefault[2];
};



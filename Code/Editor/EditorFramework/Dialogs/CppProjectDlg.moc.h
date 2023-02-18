#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_CppProjectDlg.h>
#include <Foundation/Strings/String.h>
#include <QDialog>

class EZ_EDITORFRAMEWORK_DLL ezQtCppProjectDlg : public QDialog, public Ui_ezQtCppProjectDlg
{
public:
  Q_OBJECT

public:
  ezQtCppProjectDlg(QWidget* pParent);

private Q_SLOTS:
  void on_Result_rejected();
  void on_OpenPluginLocation_clicked();
  void on_OpenBuildFolder_clicked();
  void on_Generator_currentIndexChanged(int);
  void on_OpenSolution_clicked();
  void on_GenerateSolution_clicked();

private:
  void UpdateUI();

  ezString GetTargetDir() const;
  ezString GetBuildDir() const;
  ezString GetSolutionFile() const;
  ezString GetGeneratorCMake() const;
  ezString GetGeneratorFolder() const;
  ezResult GenerateSolution();
};

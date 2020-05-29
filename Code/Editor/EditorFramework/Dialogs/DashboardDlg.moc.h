#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_DashboardDlg.h>
#include <QDialog>

class EZ_EDITORFRAMEWORK_DLL ezQtDashboardDlg : public QDialog, public Ui_ezQtDashboardDlg
{
  Q_OBJECT

public:
  ezQtDashboardDlg(QWidget* parent);

private:
  enum class DashboardTab
  {
    Projects,
    Samples,
    Documentation
  };

  void SetActiveTab(DashboardTab tab);
  void FillRecentProjectsList();
  void FindSampleProjects(ezDynamicArray<ezString>& out_Projects);
  void FillSampleProjectsList();

private Q_SLOTS:
  void on_ProjectsTab_clicked();
  void on_SamplesTab_clicked();
  void on_DocumentationTab_clicked();
  void on_NewProject_clicked();
  void on_BrowseProject_clicked();
  void on_ProjectsList_cellDoubleClicked(int row, int column);
  void on_OpenProject_clicked();
  void on_LoadLastProject_stateChanged(int);
};

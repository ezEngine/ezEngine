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
  void on_OpenSample_clicked();
  void on_LoadLastProject_stateChanged(int);
  void on_SamplesList_itemDoubleClicked(QListWidgetItem* pItem);

  void on_OpenDocs_clicked();
  void on_OpenApiDocs_clicked();
  void on_OpenReleaseNotes_clicked();
  void on_ReportProblem_clicked();
  void on_OpenGitter_clicked();
  void on_OpenTwitter_clicked();

protected:
  bool eventFilter(QObject *, QEvent *) override;
};

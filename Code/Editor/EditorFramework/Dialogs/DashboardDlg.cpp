#include <EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

ezQtDashboardDlg::ezQtDashboardDlg(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

  TabArea->tabBar()->hide();

  ProjectsTab->setFlat(true);
  SamplesTab->setFlat(true);
  DocumentationTab->setFlat(true);

  SetActiveTab(DashboardTab::Projects);
}

void ezQtDashboardDlg::SetActiveTab(DashboardTab tab)
{
  TabArea->setCurrentIndex((int)tab);

  ProjectsTab->setChecked(tab == DashboardTab::Projects);
  SamplesTab->setChecked(tab == DashboardTab::Samples);
  DocumentationTab->setChecked(tab == DashboardTab::Documentation);
}

void ezQtDashboardDlg::on_ProjectsTab_clicked()
{
  SetActiveTab(DashboardTab::Projects);
}

void ezQtDashboardDlg::on_SamplesTab_clicked()
{
  SetActiveTab(DashboardTab::Samples);
}

void ezQtDashboardDlg::on_DocumentationTab_clicked()
{
  SetActiveTab(DashboardTab::Documentation);
}

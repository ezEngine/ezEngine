#include <EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

ezQtDashboardDlg::ezQtDashboardDlg(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

  TabArea->tabBar()->hide();

  ProjectsTab->setFlat(true);
  SamplesTab->setFlat(true);
  DocumentationTab->setFlat(true);

  SetActiveTab(DashboardTab::Projects);

  FillRecentProjectsList();

  if (ProjectsList->rowCount() > 0)
  {
    ProjectsList->setFocus();
    ProjectsList->clearSelection();
    ProjectsList->selectRow(0);
  }

  if (ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>())
  {
    LoadLastProject->setChecked(pPreferences->m_bLoadLastProjectAtStartup);
  }
}

void ezQtDashboardDlg::SetActiveTab(DashboardTab tab)
{
  TabArea->setCurrentIndex((int)tab);

  ProjectsTab->setChecked(tab == DashboardTab::Projects);
  SamplesTab->setChecked(tab == DashboardTab::Samples);
  DocumentationTab->setChecked(tab == DashboardTab::Documentation);
}

void ezQtDashboardDlg::FillRecentProjectsList()
{
  const auto& list = ezQtEditorApp::GetSingleton()->GetRecentProjectsList().GetFileList();

  ProjectsList->clear();
  ProjectsList->setColumnCount(2);
  ProjectsList->setRowCount(list.GetCount());

  ezStringBuilder tmp;

  for (ezUInt32 r = 0; r < list.GetCount(); ++r)
  {
    const auto& path = list[r];

    QTableWidgetItem* pItemProjectName = new QTableWidgetItem();
    QTableWidgetItem* pItemProjectPath = new QTableWidgetItem();

    pItemProjectName->setData(Qt::UserRole, path.m_File.GetData());

    tmp = path.m_File;
    tmp.MakeCleanPath();
    tmp.PathParentDirectory(1); // remove '/ezProject'
    tmp.Trim("/");

    pItemProjectPath->setText(tmp.GetData());
    pItemProjectName->setText(tmp.GetFileName().GetStartPointer());

    ProjectsList->setItem(r, 0, pItemProjectName);
    ProjectsList->setItem(r, 1, pItemProjectPath);
  }

  ProjectsList->resizeColumnToContents(0);
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

void ezQtDashboardDlg::on_NewProject_clicked()
{
  if (ezQtEditorApp::GetSingleton()->GuiCreateProject(true))
  {
    accept();
  }
}

void ezQtDashboardDlg::on_BrowseProject_clicked()
{
  if (ezQtEditorApp::GetSingleton()->GuiOpenProject(true))
  {
    accept();
  }
}

void ezQtDashboardDlg::on_ProjectsList_cellDoubleClicked(int row, int column)
{
  if (row < 0 || row >= ProjectsList->rowCount())
    return;

  QTableWidgetItem* pItem = ProjectsList->item(row, 0);

  QString sPath = pItem->data(Qt::UserRole).toString();

  if (ezQtEditorApp::GetSingleton()->OpenProject(sPath.toUtf8().data(), true).Succeeded())
  {
    accept();
  }
}

void ezQtDashboardDlg::on_OpenProject_clicked()
{
  on_ProjectsList_cellDoubleClicked(ProjectsList->currentRow(), 0);
}

void ezQtDashboardDlg::on_LoadLastProject_stateChanged(int)
{
  if (ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>())
  {
    pPreferences->m_bLoadLastProjectAtStartup = LoadLastProject->isChecked();
  }
}

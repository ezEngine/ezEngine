#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

ezQtDashboardDlg::ezQtDashboardDlg(QWidget* pParent, DashboardTab activeTab)
  : QDialog(pParent)
{
  setupUi(this);

  TabArea->tabBar()->hide();

  ProjectsTab->setFlat(true);
  SamplesTab->setFlat(true);
  DocumentationTab->setFlat(true);

  if (ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>())
  {
    LoadLastProject->setChecked(pPreferences->m_bLoadLastProjectAtStartup);
  }

  {
    SamplesList->setResizeMode(QListView::ResizeMode::Adjust);
    SamplesList->setIconSize(QSize(220, 220));
    SamplesList->setItemAlignment(Qt::AlignHCenter | Qt::AlignBottom);
  }

  FillRecentProjectsList();
  FillSampleProjectsList();

  ProjectsList->installEventFilter(this);

  if (ProjectsList->rowCount() > 0)
  {
    ProjectsList->setFocus();
    ProjectsList->clearSelection();
    ProjectsList->selectRow(0);
  }
  else
  {
    if (activeTab == DashboardTab::Projects)
    {
      activeTab = DashboardTab::Samples;
    }
  }

  SetActiveTab(activeTab);
}

void ezQtDashboardDlg::SetActiveTab(DashboardTab activeTab)
{
  TabArea->setCurrentIndex((int)activeTab);

  ProjectsTab->setChecked(activeTab == DashboardTab::Projects);
  SamplesTab->setChecked(activeTab == DashboardTab::Samples);
  DocumentationTab->setChecked(activeTab == DashboardTab::Documentation);
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

void ezQtDashboardDlg::FillSampleProjectsList()
{
  ezHybridArray<ezString, 32> samples;
  FindSampleProjects(samples);

  SamplesList->clear();

  ezStringBuilder tmp, iconPath;

  ezStringBuilder samplesIcon = ezApplicationServices::GetSingleton()->GetSampleProjectsFolder();
  samplesIcon.AppendPath("Thumbnail.jpg");

  QIcon fallbackIcon;

  if (ezOSFile::ExistsFile(samplesIcon))
  {
    fallbackIcon.addFile(samplesIcon.GetData());
  }

  for (const ezString& path : samples)
  {
    tmp = path;
    const bool bIsLocal = tmp.TrimWordEnd("/ezProject");
    const bool bIsRemote = tmp.TrimWordEnd("/ezRemoteProject");

    QIcon projectIcon;

    iconPath = tmp;
    iconPath.AppendPath("Thumbnail.jpg");

    if (ezOSFile::ExistsFile(iconPath))
    {
      projectIcon.addFile(iconPath.GetData());
    }
    else
    {
      projectIcon = fallbackIcon;
    }

    QListWidgetItem* pItem = new QListWidgetItem();
    pItem->setText(tmp.GetFileName().GetStartPointer());
    pItem->setData(Qt::UserRole, path.GetData());

    pItem->setIcon(projectIcon);

    SamplesList->addItem(pItem);
  }
}

void ezQtDashboardDlg::FindSampleProjects(ezDynamicArray<ezString>& out_Projects)
{
  out_Projects.Clear();

  const ezString& sSampleProjects = ezApplicationServices::GetSingleton()->GetSampleProjectsFolder();

  ezFileSystemIterator fsIt;
  fsIt.StartSearch(sSampleProjects, ezFileSystemIteratorFlags::ReportFoldersRecursive);

  ezStringBuilder path;

  while (fsIt.IsValid())
  {
    fsIt.GetStats().GetFullPath(path);
    path.AppendPath("ezProject");

    if (ezOSFile::ExistsFile(path))
    {
      out_Projects.PushBack(path);

      // no need to go deeper
      fsIt.SkipFolder();
    }
    else
    {
      fsIt.GetStats().GetFullPath(path);
      path.AppendPath("ezRemoteProject");

      if (ezOSFile::ExistsFile(path))
      {
        out_Projects.PushBack(path);

        // no need to go deeper
        fsIt.SkipFolder();
      }
      else
      {
        fsIt.Next();
      }
    }
  }
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

void ezQtDashboardDlg::on_OpenSample_clicked()
{
  on_SamplesList_itemDoubleClicked(SamplesList->currentItem());
}

void ezQtDashboardDlg::on_LoadLastProject_stateChanged(int)
{
  if (ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>())
  {
    pPreferences->m_bLoadLastProjectAtStartup = LoadLastProject->isChecked();
  }
}

void ezQtDashboardDlg::on_SamplesList_itemDoubleClicked(QListWidgetItem* pItem)
{
  if (pItem == nullptr)
    return;

  QString sPath = pItem->data(Qt::UserRole).toString().toUtf8().data();

  if (ezQtEditorApp::GetSingleton()->OpenProject(sPath.toUtf8().data(), true).Succeeded())
  {
    accept();
  }
}

void ezQtDashboardDlg::on_OpenDocs_clicked()
{
  QDesktopServices::openUrl(QUrl("https://ezengine.net"));
}

void ezQtDashboardDlg::on_OpenApiDocs_clicked()
{
  QDesktopServices::openUrl(QUrl("https://ezengine.github.io/api-docs/"));
}

void ezQtDashboardDlg::on_GitHubDiscussions_clicked()
{
  QDesktopServices::openUrl(QUrl("https://github.com/ezEngine/ezEngine/discussions"));
}

void ezQtDashboardDlg::on_ReportProblem_clicked()
{
  QDesktopServices::openUrl(QUrl("https://github.com/ezEngine/ezEngine/issues"));
}

void ezQtDashboardDlg::on_OpenDiscord_clicked()
{
  QDesktopServices::openUrl(QUrl("https://discord.gg/rfJewc5khZ"));
}

void ezQtDashboardDlg::on_OpenTwitter_clicked()
{
  QDesktopServices::openUrl(QUrl("https://twitter.com/ezEngineProject"));
}

void ezQtDashboardDlg::on_OpenBsky_clicked()
{
  QDesktopServices::openUrl(QUrl("https://bsky.app/profile/ezengine.bsky.social"));
}

bool ezQtDashboardDlg::eventFilter(QObject* obj, QEvent* e)
{
  if (e->type() == QEvent::Type::KeyPress)
  {
    QKeyEvent* key = static_cast<QKeyEvent*>(e);

    if ((key->key() == Qt::Key_Enter) || (key->key() == Qt::Key_Return))
    {
      on_OpenProject_clicked();
      return true;
    }
  }

  return QObject::eventFilter(obj, e);
}

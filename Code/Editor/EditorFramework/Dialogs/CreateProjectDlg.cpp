#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/CreateProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>


ezQtCreateProjectDlg::ezQtCreateProjectDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  m_sTargetFolder = ezApplicationServices::GetSingleton()->GetSampleProjectsFolder().GetData();

  UpdateeUI();
}

ezString ezQtCreateProjectDlg::GetFullTargetPath() const
{
  ezStringBuilder name = m_sTargetName;

  name.Trim();

  if (name.IsEmpty())
    return {};

  ezStringBuilder path;
  path.SetPath(m_sTargetFolder, m_sTargetName);

  return path;
}

void ezQtCreateProjectDlg::UpdateeUI()
{
  ezQtScopedBlockSignals _1(ProjectFolder);
  ezQtScopedBlockSignals _2(ProjectName);

  ProjectFolder->setText(m_sTargetFolder.GetData());

  ezString sFullPath = GetFullTargetPath();

  if (sFullPath.IsEmpty() || !sFullPath.IsAbsolutePath())
  {
    ResultPath->setText("<Choose a name and parent folder>");
    CreateProject->setEnabled(false);
  }
  else if (ezOSFile::ExistsDirectory(sFullPath))
  {
    // ResultPath->setColor(qRgb(255, 0, 0));
    ResultPath->setText("Directory already exists");
    CreateProject->setEnabled(false);
  }
  else
  {
    // ResultPath->setColor(qRgb(0, 255, 0));
    ResultPath->setText(sFullPath.GetData());
    CreateProject->setEnabled(!sFullPath.IsEmpty());
  }
}

void ezQtCreateProjectDlg::on_BrowseFolder_clicked()
{
  QString sFile = QFileDialog::getExistingDirectory(QApplication::activeWindow(), "Choose Folder", m_sTargetFolder.GetData(), QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  m_sTargetFolder = sFile.toUtf8().data();

  UpdateeUI();
}

void ezQtCreateProjectDlg::on_ProjectName_textChanged(QString text)
{
  m_sTargetName = ProjectName->text().toUtf8().data();

  UpdateeUI();
}

void ezQtCreateProjectDlg::on_CreateProject_clicked()
{
  const ezString sFullPath = GetFullTargetPath();

  if (ezOSFile::CreateDirectoryStructure(sFullPath).Failed())
  {
  }

  QDialog::accept();
}

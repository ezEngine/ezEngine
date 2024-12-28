#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/CreateProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

ezQtCreateProjectDlg::ezQtCreateProjectDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  Prev->setVisible(false);

  m_sTargetFolder = ezApplicationServices::GetSingleton()->GetSampleProjectsFolder().GetData();

  ezQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(ezOSFile::GetApplicationDirectory());
  m_LocalPluginSet = ezQtEditorApp::GetSingleton()->GetPluginBundles();
  m_LocalPluginSet.SetFromTemplate("General3D");

  Plugins->SetPluginSet(&m_LocalPluginSet);
  Plugins->SelectTemplate("General3D");

  UpdateUI();
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

void ezQtCreateProjectDlg::UpdateUI()
{
  ezQtScopedBlockSignals _1(ProjectFolder);
  ezQtScopedBlockSignals _2(ProjectName);

  ProjectFolder->setText(m_sTargetFolder.GetData());

  ezString sFullPath = GetFullTargetPath();

  if (sFullPath.IsEmpty() || !sFullPath.IsAbsolutePath())
  {
    ResultPath->setText("<Choose a name and parent folder>");
    Next->setEnabled(false);
  }
  else if (ezOSFile::ExistsDirectory(sFullPath))
  {
    // ResultPath->setColor(qRgb(255, 0, 0));
    ResultPath->setText("Directory already exists");
    Next->setEnabled(false);
  }
  else
  {
    // ResultPath->setColor(qRgb(0, 255, 0));
    ResultPath->setText(sFullPath.GetData());
    Next->setEnabled(!sFullPath.IsEmpty());
  }

  switch (m_state)
  {
    case State::Basics:
      StackedPages->setCurrentIndex(0);
      Prev->setVisible(false);
      Next->setText("Next >");
      break;

    case State::Plugins:
      StackedPages->setCurrentIndex(1);
      Prev->setVisible(true);
      Next->setText("Create");
      break;
  }
}

void ezQtCreateProjectDlg::on_BrowseFolder_clicked()
{
  QString sFile = QFileDialog::getExistingDirectory(QApplication::activeWindow(), "Choose Folder", m_sTargetFolder.GetData(), QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  m_sTargetFolder = sFile.toUtf8().data();

  UpdateUI();
}

void ezQtCreateProjectDlg::on_ProjectName_textChanged(QString text)
{
  m_sTargetName = ProjectName->text().toUtf8().data();

  UpdateUI();
}

void ezQtCreateProjectDlg::on_Prev_clicked()
{
  switch (m_state)
  {
    case State::Plugins:
      m_state = State::Basics;
      break;
  }

  UpdateUI();
}

void ezQtCreateProjectDlg::on_Next_clicked()
{
  switch (m_state)
  {
    case State::Basics:
      m_state = State::Plugins;
      break;

    case State::Plugins:
      m_state = State::Create;
      break;
  }

  UpdateUI();

  if (m_state == State::Create)
  {
    const ezString sFullPath = GetFullTargetPath();

    if (ezOSFile::CreateDirectoryStructure(sFullPath).Failed())
    {
    }

    Plugins->SyncStateToSet();

    {
      ezStringBuilder path = sFullPath;
      path.AppendPath("Editor/PluginSelection.ddl");

      ezFileWriter file;
      file.Open(path).AssertSuccess();

      ezOpenDdlWriter ddl;
      ddl.SetOutputStream(&file);

      m_LocalPluginSet.WriteStateToDDL(ddl);
    }

    QDialog::accept();
  }
}

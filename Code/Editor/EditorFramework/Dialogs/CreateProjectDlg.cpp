#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/CreateProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Project/ProjectCreation.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

ezQtCreateProjectDlg::ezQtCreateProjectDlg(QWidget* pParent)
  : ezQtDialog(pParent)
{
  setupUi(this);

  Prev->setVisible(false);

  m_sTargetFolder = ezApplicationServices::GetSingleton()->GetSampleProjectsFolder().GetData();

  ezQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(ezOSFile::GetApplicationDirectory());
  m_LocalPluginSet = ezQtEditorApp::GetSingleton()->GetPluginBundles();
  m_LocalPluginSet.SetFromTemplate("General3D");

  Plugins->SetPluginSet(&m_LocalPluginSet);
  Plugins->SelectTemplate("General3D");

  ProjectTemplates->setResizeMode(QListView::ResizeMode::Adjust);
  ProjectTemplates->setIconSize(QSize(220, 220));
  ProjectTemplates->setItemAlignment(Qt::AlignHCenter | Qt::AlignBottom);

  UpdateUI();

  FillProjectTemplatesList();
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

  if (m_sProjectTemplate.IsEmpty())
    ChosenTemplate->setText("<none>");
  else
  {
    ezStringBuilder tmp = m_sProjectTemplate;
    tmp.PathParentDirectory();
    tmp.TrimRight("/\\");

    ChosenTemplate->setText(ezMakeQString(tmp.GetFileName()));
  }

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
    ResultPath2->setText(sFullPath.GetData());
    Next->setEnabled(!sFullPath.IsEmpty());
  }

  switch (m_State)
  {
    case State::Basics:
      StackedPages->setCurrentIndex(0);
      Prev->setVisible(false);
      Next->setText("Next >");
      break;

    case State::Templates:
      StackedPages->setCurrentIndex(2);
      Prev->setVisible(true);
      Next->setText("Next >");
      break;

    case State::Plugins:
      StackedPages->setCurrentIndex(1);
      Prev->setVisible(true);
      Next->setText("Next >");
      break;

    case State::Summary:
      StackedPages->setCurrentIndex(3);
      Prev->setVisible(true);
      Next->setText("Create");
      break;

    case State::Create:
      break;
  }
}

void ezQtCreateProjectDlg::FillProjectTemplatesList()
{
  ezDynamicArray<ezString> templateNames;
  ezProjectCreation::FindProjectTemplates(templateNames);

  ezTempHybridArray<ezString, 32> templates;
  for (const ezString& sName : templateNames)
  {
    ezStringBuilder sProjectFile;
    if (ezProjectCreation::FindProjectTemplate(sName, sProjectFile).Succeeded())
    {
      templates.PushBack(sProjectFile);
    }
  }

  ProjectTemplates->clear();

  ezStringBuilder tmp, iconPath;

  ezStringBuilder samplesIcon = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
  samplesIcon.AppendPath("ProjectTemplates/Thumbnail.jpg");

  QIcon fallbackIcon;

  if (ezOSFile::ExistsFile(samplesIcon))
  {
    fallbackIcon.addFile(samplesIcon.GetData());
  }

  {
    QListWidgetItem* pItem = new QListWidgetItem();
    pItem->setText("Blank Project");
    pItem->setData(Qt::UserRole, QString());

    pItem->setIcon(fallbackIcon);

    ProjectTemplates->addItem(pItem);

    ProjectTemplates->setCurrentItem(pItem);

    pItem->setSelected(true);
  }

  for (const ezString& path : templates)
  {
    tmp = path;
    const bool bIsLocal = tmp.TrimWordEnd("/ezProject");
    // const bool bIsRemote = tmp.TrimWordEnd("/ezRemoteProject");

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

    ProjectTemplates->addItem(pItem);
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
  switch (m_State)
  {
    case State::Templates:
      m_State = State::Basics;
      break;

    case State::Plugins:
      m_State = State::Templates;
      break;

    case State::Summary:
      if (m_sProjectTemplate.IsEmpty())
        m_State = State::Plugins;
      else
        m_State = State::Templates;
      break;

    default:
      break;
  }

  UpdateUI();
}

void ezQtCreateProjectDlg::on_Next_clicked()
{
  switch (m_State)
  {
    case State::Basics:
      m_State = State::Templates;
      break;

    case State::Templates:
    {
      // no current item -> nothign selected -> blank project
      if (ProjectTemplates->currentItem())
      {
        m_sProjectTemplate = ProjectTemplates->currentItem()->data(Qt::UserRole).toString().toUtf8().data();
      }

      if (m_sProjectTemplate.IsEmpty())
        m_State = State::Plugins;
      else
        m_State = State::Summary;

      break;
    }

    case State::Plugins:
      Plugins->SyncStateToSet();
      m_State = State::Summary;
      break;

    case State::Summary:
      m_State = State::Create;
      break;
    case State::Create:
      break;
  }

  UpdateUI();

  if (m_State == State::Create)
  {
    const ezStatus res = CreateProject();

    if (res.Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Creating the project failed.");

      // back to the summary page, so that the user can change the name or folder and try again
      m_State = State::Summary;
      UpdateUI();
      return;
    }

    QDialog::accept();
  }
}

ezStatus ezQtCreateProjectDlg::CreateProject()
{
  ezProjectCreationOptions options;
  options.m_sTargetDirectory = GetFullTargetPath();

  if (m_sProjectTemplate.IsEmpty())
  {
    // the plugin page wrote its state into m_LocalPluginSet, so pass that on as it is rather than
    // letting the creation apply a plugin template again
    return ezProjectCreation::CreateProject(options, m_LocalPluginSet);
  }

  // m_sProjectTemplate is the path of the template's 'ezProject' file, the creation takes the name
  ezStringBuilder sTemplateName = m_sProjectTemplate;
  sTemplateName.PathParentDirectory();
  sTemplateName.TrimRight("/\\");
  options.m_sProjectTemplate = sTemplateName.GetFileName();

  return ezProjectCreation::CreateProject(options, m_LocalPluginSet);
}

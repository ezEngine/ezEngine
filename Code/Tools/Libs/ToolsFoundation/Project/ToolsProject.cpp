#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Project/ToolsProject.h>

EZ_IMPLEMENT_SINGLETON(ezToolsProject);

ezEvent<const ezToolsProjectEvent&, ezMutex> ezToolsProject::s_Events;
ezEvent<ezToolsProjectRequest&> ezToolsProject::s_Requests;


ezToolsProjectRequest::ezToolsProjectRequest()
{
  m_Type = Type::CanCloseProject;
  m_bCanClose = true;
  m_iContainerWindowUniqueIdentifier = 0;
}

ezToolsProject::ezToolsProject(ezStringView sProjectPath)
  : m_SingletonRegistrar(this)
{
  m_bIsClosing = false;

  m_sProjectPath = sProjectPath;
  EZ_ASSERT_DEV(!m_sProjectPath.IsEmpty(), "Path cannot be empty.");
}

ezToolsProject::~ezToolsProject() = default;

ezStatus ezToolsProject::Create()
{
  {
    ezOSFile ProjectFile;
    if (ProjectFile.Open(m_sProjectPath, ezFileOpenMode::Write).Failed())
    {
      return ezStatus(ezFmt("Could not open/create the project file for writing: '{0}'", m_sProjectPath));
    }
    else
    {
      ezStringView szToken = "ezEditor Project File";

      EZ_SUCCEED_OR_RETURN(ProjectFile.Write(szToken.GetStartPointer(), szToken.GetElementCount() + 1));
      ProjectFile.Close();
    }
  }

  {
    ezToolsProjectEvent e;
    e.m_pProject = this;
    e.m_Type = ezToolsProjectEvent::Type::ProjectCreated;
    s_Events.Broadcast(e);
  }

  EZ_SUCCEED_OR_RETURN(Open());

  // if this file already exists, the project was created from a template and should not get additional setup
  ezStringBuilder path(GetProjectDirectory(), "/Scenes/Main.ezScene");
  if (!ezOSFile::ExistsDirectory(path))
  {
    ezToolsProjectEvent e;
    e.m_pProject = this;
    e.m_Type = ezToolsProjectEvent::Type::ProjectFirstSetup;
    s_Events.Broadcast(e);
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezToolsProject::Open()
{
  ezOSFile ProjectFile;
  if (ProjectFile.Open(m_sProjectPath, ezFileOpenMode::Read).Failed())
  {
    return ezStatus(ezFmt("Could not open the project file for reading: '{0}'", m_sProjectPath));
  }

  ProjectFile.Close();

  ezToolsProjectEvent e;
  e.m_pProject = this;
  e.m_Type = ezToolsProjectEvent::Type::ProjectOpened;
  s_Events.Broadcast(e);

  return ezStatus(EZ_SUCCESS);
}

void ezToolsProject::CreateSubFolder(ezStringView sFolder) const
{
  ezStringBuilder sPath;

  sPath = m_sProjectPath;
  sPath.PathParentDirectory();
  sPath.AppendPath(sFolder);

  ezOSFile::CreateDirectoryStructure(sPath).IgnoreResult();
}

void ezToolsProject::CloseProject()
{
  if (GetSingleton())
  {
    GetSingleton()->m_bIsClosing = true;

    ezToolsProjectEvent e;
    e.m_pProject = GetSingleton();
    e.m_Type = ezToolsProjectEvent::Type::ProjectClosing;
    s_Events.Broadcast(e);

    ezDocumentManager::CloseAllDocuments();

    delete GetSingleton();

    e.m_Type = ezToolsProjectEvent::Type::ProjectClosed;
    s_Events.Broadcast(e);
  }
}

void ezToolsProject::SaveProjectState()
{
  if (GetSingleton())
  {
    GetSingleton()->m_bIsClosing = true;

    ezToolsProjectEvent e;
    e.m_pProject = GetSingleton();
    e.m_Type = ezToolsProjectEvent::Type::ProjectSaveState;
    s_Events.Broadcast(e, 1);
  }
}

bool ezToolsProject::CanCloseProject()
{
  if (GetSingleton() == nullptr)
    return true;

  ezToolsProjectRequest e;
  e.m_Type = ezToolsProjectRequest::Type::CanCloseProject;
  e.m_bCanClose = true;
  s_Requests.Broadcast(e, 1); // when the save dialog pops up and the user presses 'Save' we need to allow one more recursion

  return e.m_bCanClose;
}

bool ezToolsProject::CanCloseDocuments(ezArrayPtr<ezDocument*> documents)
{
  if (GetSingleton() == nullptr)
    return true;

  ezToolsProjectRequest e;
  e.m_Type = ezToolsProjectRequest::Type::CanCloseDocuments;
  e.m_bCanClose = true;
  e.m_Documents = documents;
  s_Requests.Broadcast(e);

  return e.m_bCanClose;
}

ezInt32 ezToolsProject::SuggestContainerWindow(ezDocument* pDoc)
{
  if (pDoc == nullptr)
  {
    return 0;
  }
  ezToolsProjectRequest e;
  e.m_Type = ezToolsProjectRequest::Type::SuggestContainerWindow;
  e.m_Documents.PushBack(pDoc);
  s_Requests.Broadcast(e);

  return e.m_iContainerWindowUniqueIdentifier;
}

ezStringBuilder ezToolsProject::GetPathForDocumentGuid(const ezUuid& guid)
{
  ezToolsProjectRequest e;
  e.m_Type = ezToolsProjectRequest::Type::GetPathForDocumentGuid;
  e.m_documentGuid = guid;
  s_Requests.Broadcast(e, 1); // this can be sent while CanCloseProject is processed, so allow one additional recursion depth
  return e.m_sAbsDocumentPath;
}

ezStatus ezToolsProject::CreateOrOpenProject(ezStringView sProjectPath, bool bCreate)
{
  CloseProject();

  new ezToolsProject(sProjectPath);

  ezStatus ret;

  if (bCreate)
  {
    ret = GetSingleton()->Create();
    ezToolsProject::SaveProjectState();
  }
  else
    ret = GetSingleton()->Open();

  if (ret.m_Result.Failed())
  {
    delete GetSingleton();
    return ret;
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezToolsProject::OpenProject(ezStringView sProjectPath)
{
  ezStatus status = CreateOrOpenProject(sProjectPath, false);

  return status;
}

ezStatus ezToolsProject::CreateProject(ezStringView sProjectPath)
{
  return CreateOrOpenProject(sProjectPath, true);
}

void ezToolsProject::BroadcastSaveAll()
{
  ezToolsProjectEvent e;
  e.m_pProject = GetSingleton();
  e.m_Type = ezToolsProjectEvent::Type::SaveAll;

  s_Events.Broadcast(e);
}

void ezToolsProject::BroadcastConfigChanged()
{
  ezToolsProjectEvent e;
  e.m_pProject = GetSingleton();
  e.m_Type = ezToolsProjectEvent::Type::ProjectConfigChanged;

  s_Events.Broadcast(e);
}

void ezToolsProject::AddAllowedDocumentRoot(ezStringView sPath)
{
  ezStringBuilder s = sPath;
  s.MakeCleanPath();
  s.Trim("", "/");

  m_AllowedDocumentRoots.PushBack(s);
}


bool ezToolsProject::IsDocumentInAllowedRoot(ezStringView sDocumentPath, ezString* out_pRelativePath) const
{
  for (ezUInt32 i = m_AllowedDocumentRoots.GetCount(); i > 0; --i)
  {
    const auto& root = m_AllowedDocumentRoots[i - 1];

    ezStringBuilder s = sDocumentPath;
    if (!s.IsPathBelowFolder(root))
      continue;

    if (out_pRelativePath)
    {
      ezStringBuilder sText = sDocumentPath;
      sText.MakeRelativeTo(root).IgnoreResult();

      *out_pRelativePath = sText;
    }

    return true;
  }

  return false;
}

const ezString ezToolsProject::GetProjectName(bool bSanitize) const
{
  ezStringBuilder sTemp = ezToolsProject::GetSingleton()->GetProjectFile();
  sTemp.PathParentDirectory();
  sTemp.Trim("/");

  if (!bSanitize)
    return sTemp.GetFileName();

  const ezStringBuilder sOrgName = sTemp.GetFileName();
  sTemp.Clear();

  bool bAnyAscii = false;

  for (ezStringIterator it = sOrgName.GetIteratorFront(); it.IsValid(); ++it)
  {
    const ezUInt32 c = it.GetCharacter();

    if (!ezStringUtils::IsIdentifierDelimiter_C_Code(c))
    {
      bAnyAscii = true;

      // valid character to be used in C as an identifier
      sTemp.Append(c);
    }
    else if (c == ' ')
    {
      // skip
    }
    else
    {
      sTemp.AppendFormat("{}", ezArgU(c, 1, false, 16));
    }
  }

  if (!bAnyAscii)
  {
    const ezUInt32 uiHash = ezHashingUtils::xxHash32String(sTemp);
    sTemp.SetFormat("Project{}", uiHash);
  }

  if (sTemp.IsEmpty())
  {
    sTemp = "Project";
  }

  if (sTemp.GetCharacterCount() > 20)
  {
    sTemp.Shrink(0, sTemp.GetCharacterCount() - 20);
  }

  return sTemp;
}

ezString ezToolsProject::GetProjectDirectory() const
{
  ezStringBuilder s = GetProjectFile();

  s.PathParentDirectory();
  s.Trim("", "/\\");

  return s;
}

ezString ezToolsProject::GetProjectDataFolder() const
{
  ezStringBuilder s = GetProjectDirectory();
  s.AppendPath("Editor");

  return s;
}

ezString ezToolsProject::FindProjectDirectoryForDocument(ezStringView sDocumentPath)
{
  ezStringBuilder sPath = sDocumentPath;
  sPath.PathParentDirectory();

  ezStringBuilder sTemp;

  while (!sPath.IsEmpty())
  {
    sTemp = sPath;
    sTemp.AppendPath("ezProject");

    if (ezOSFile::ExistsFile(sTemp))
      return sPath;

    sPath.PathParentDirectory();
  }

  return "";
}

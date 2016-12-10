#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <Foundation/IO/OSFile.h>

EZ_IMPLEMENT_SINGLETON(ezToolsProject);

ezEvent<const ezToolsProjectEvent&> ezToolsProject::s_Events;
ezEvent<ezToolsProjectRequest&> ezToolsProject::s_Requests;


ezToolsProjectRequest::ezToolsProjectRequest()
{
  m_Type = Type::CanCloseProject;
  m_bCanClose = true;
  m_iContainerWindowUniqueIdentifier = 0;
}

ezToolsProject::ezToolsProject(const char* szProjectPath)
  : m_SingletonRegistrar(this)
{
  m_bIsClosing = false;

  m_sProjectPath = szProjectPath;
  EZ_ASSERT_DEV(!m_sProjectPath.IsEmpty(), "Path cannot be empty.");
}

ezToolsProject::~ezToolsProject()
{
}

ezStatus ezToolsProject::Create()
{
  {
    ezOSFile ProjectFile;
    if (ProjectFile.Open(m_sProjectPath, ezFileMode::Write).Failed())
    {
      return ezStatus(ezFmt("Could not open/create the project file for writing: '{0}'", m_sProjectPath.GetData()));
    }
    else
    {
      const char* szToken = "ezEditor Project File";

      ProjectFile.Write(szToken, ezStringUtils::GetStringElementCount(szToken) + 1);
      ProjectFile.Close();
    }
  }

  // Create default folders
  {
    CreateSubFolder("Scenes");
    CreateSubFolder("Prefabs");
  }

  ezToolsProjectEvent e;
  e.m_pProject = this;
  e.m_Type = ezToolsProjectEvent::Type::ProjectCreated;
  s_Events.Broadcast(e);

  return Open();
}

ezStatus ezToolsProject::Open()
{
  ezOSFile ProjectFile;
  if (ProjectFile.Open(m_sProjectPath, ezFileMode::Read).Failed())
  {
    return ezStatus(ezFmt("Could not open the project file for reading: '{0}'", m_sProjectPath.GetData()));
  }

  ProjectFile.Close();

  ezToolsProjectEvent e;
  e.m_pProject = this;
  e.m_Type = ezToolsProjectEvent::Type::ProjectOpened;
  s_Events.Broadcast(e);

  return ezStatus(EZ_SUCCESS);
}

void ezToolsProject::CreateSubFolder(const char* szFolder) const
{
  ezStringBuilder sPath;

  sPath = m_sProjectPath;
  sPath.PathParentDirectory();
  sPath.AppendPath(szFolder);

  ezOSFile::CreateDirectoryStructure(sPath);
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

bool ezToolsProject::CanCloseProject()
{
  if (GetSingleton() == nullptr)
    return true;

  ezToolsProjectRequest e;
  e.m_Type = ezToolsProjectRequest::Type::CanCloseProject;
  e.m_bCanClose = true;
  s_Requests.Broadcast(e);

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
  s_Requests.Broadcast(e);
  return e.m_sAbsDocumentPath;
}

ezStatus ezToolsProject::CreateOrOpenProject(const char* szProjectPath, bool bCreate)
{
  CloseProject();

  new ezToolsProject(szProjectPath);

  ezStatus ret;

  if (bCreate)
    ret = GetSingleton()->Create();
  else
    ret = GetSingleton()->Open();

  if (ret.m_Result.Failed())
  {
    delete GetSingleton();
    return ret;
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezToolsProject::OpenProject(const char* szProjectPath)
{
  ezStatus status = CreateOrOpenProject(szProjectPath, false);

  return status;
}

ezStatus ezToolsProject::CreateProject(const char* szProjectPath)
{
  return CreateOrOpenProject(szProjectPath, true);
}

void ezToolsProject::BroadcastConfigChanged()
{
  ezToolsProjectEvent e;
  e.m_pProject = GetSingleton();
  e.m_Type = ezToolsProjectEvent::Type::ProjectConfigChanged;

  s_Events.Broadcast(e);
}

void ezToolsProject::AddAllowedDocumentRoot(const char* szPath)
{
  ezStringBuilder s = szPath;
  s.MakeCleanPath();
  s.Trim("", "/");

  m_AllowedDocumentRoots.PushBack(s);
}


bool ezToolsProject::IsDocumentInAllowedRoot(const char* szDocumentPath, ezString* out_RelativePath) const
{
  for (ezUInt32 i = m_AllowedDocumentRoots.GetCount(); i > 0; --i)
  {
    const auto& root = m_AllowedDocumentRoots[i - 1];

    ezStringBuilder s = szDocumentPath;
    if (!s.IsPathBelowFolder(root))
      continue;

    if (out_RelativePath)
    {
      const ezInt32 iTrimStart = root.GetCharacterCount();

      ezStringBuilder sText = szDocumentPath;
      sText.MakeRelativeTo(root);

      *out_RelativePath = sText;
    }

    return true;
  }

  return false;
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
  ezStringBuilder s = GetProjectFile();
  s.Append("_data");

  return s;
}

ezString ezToolsProject::FindProjectDirectoryForDocument(const char* szDocumentPath)
{
  ezStringBuilder sPath = szDocumentPath;
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

#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Document/DocumentManager.h>

ezToolsProject* ezToolsProject::s_pInstance = nullptr;
ezEvent<const ezToolsProject::Event&> ezToolsProject::s_Events;
ezEvent<ezToolsProject::Request&> ezToolsProject::s_Requests;

ezToolsProject::ezToolsProject(const char* szProjectPath)
{
  EZ_ASSERT_DEV(s_pInstance == nullptr, "There can be only one");

  s_pInstance = this;
  m_bIsClosing = false;

  m_sProjectPath = szProjectPath;
  EZ_ASSERT_DEV(!m_sProjectPath.IsEmpty(), "Path cannot be empty.");
}

ezToolsProject::~ezToolsProject()
{
  s_pInstance = nullptr;
}

ezStatus ezToolsProject::Create()
{
  {
    ezOSFile ProjectFile;
    if (ProjectFile.Open(m_sProjectPath, ezFileMode::Write).Failed())
    {
      ezStringBuilder sError;
      sError.Format("Could not open/create the project file for writing: '%s'", m_sProjectPath.GetData());
      return ezStatus(sError);
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
    ezStringBuilder sPath;

    sPath = m_sProjectPath;
    sPath.PathParentDirectory();
    sPath.AppendPath("Scenes");

    ezOSFile::CreateDirectoryStructure(sPath);
  }

  Event e;
  e.m_Type = Event::Type::ProjectCreated;
  s_Events.Broadcast(e);

  return Open();
}

ezStatus ezToolsProject::Open()
{
  ezOSFile ProjectFile;
  if (ProjectFile.Open(m_sProjectPath, ezFileMode::Read).Failed())
  {
    ezStringBuilder sError;
    sError.Format("Could not open the project file for reading: '%s'", m_sProjectPath.GetData());
    return ezStatus(sError);
  }

  ProjectFile.Close();

  Event e;
  e.m_Type = Event::Type::ProjectOpened;
  s_Events.Broadcast(e);

  e.m_Type = Event::Type::ProjectOpened2;
  s_Events.Broadcast(e);

  return ezStatus(EZ_SUCCESS);
}

void ezToolsProject::CloseProject()
{
  if (s_pInstance)
  {
    s_pInstance->m_bIsClosing = true;

    Event e;
    e.m_Type = Event::Type::ProjectClosing;
    s_Events.Broadcast(e);

    ezDocumentManagerBase::CloseAllDocuments();

    delete s_pInstance;

    e.m_Type = Event::Type::ProjectClosed;
    s_Events.Broadcast(e);
  }
}

bool ezToolsProject::CanCloseProject()
{
  if (GetInstance() == nullptr)
    return true;

  Request e;
  e.m_Type = Request::Type::CanProjectClose;
  e.m_bProjectCanClose = true;
  s_Requests.Broadcast(e);

  return e.m_bProjectCanClose;
}

ezStatus ezToolsProject::CreateOrOpenProject(const char* szProjectPath, bool bCreate)
{
  CloseProject();

  new ezToolsProject(szProjectPath);

  ezStatus ret;

  if (bCreate)
    ret = s_pInstance->Create();
  else
    ret = s_pInstance->Open();

  if (ret.m_Result.Failed())
  {
    delete s_pInstance;
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

ezString ezToolsProject::FindProjectForDocument(const char* szDocumentPath)
{
#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)
  ezStringBuilder sPath = szDocumentPath;
  sPath.PathParentDirectory();

  ezStringBuilder sTemp;

  while (!sPath.IsEmpty())
  {
    sTemp = sPath;
    sTemp.AppendPath("ezProject");

    if (ezOSFile::ExistsFile(sTemp))
      return sTemp;

    sPath.PathParentDirectory();
  }
#endif

  return "";
}

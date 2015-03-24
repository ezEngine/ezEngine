#include <PCH.h>
#include <EditorFramework/EditorApp.moc.h>
#include <EditorFramework/Project/EditorProject.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>

ezEditorProject* ezEditorProject::s_pInstance = nullptr;
ezEvent<const ezEditorProject::Event&> ezEditorProject::s_Events;
ezEvent<ezEditorProject::Request&> ezEditorProject::s_Requests;

ezEditorProject::ezEditorProject(const char* szProjectPath)
{
  EZ_ASSERT_DEV(s_pInstance == nullptr, "There can be only one");
  
  s_pInstance = this;

  m_sProjectPath = szProjectPath;
  EZ_ASSERT_DEV(!m_sProjectPath.IsEmpty(), "Path cannot be empty.");
}

ezEditorProject::~ezEditorProject()
{
  s_pInstance = nullptr;
}

ezStatus ezEditorProject::Create()
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

ezStatus ezEditorProject::Open()
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

  return ezStatus(EZ_SUCCESS);
}

void ezEditorProject::CloseProject()
{
  if (s_pInstance)
  {
    Event e;
    e.m_Type = Event::Type::ProjectClosing;
    s_Events.Broadcast(e);

    ezDocumentManagerBase::CloseAllDocuments();

    delete s_pInstance;

    e.m_Type = Event::Type::ProjectClosed;
    s_Events.Broadcast(e);

    ezEditorEngineProcessConnection::GetInstance()->ShutdownProcess();
  }
}

bool ezEditorProject::CanCloseProject()
{
  if (GetInstance() == nullptr)
    return true;

  Request e;
  e.m_Type = Request::Type::CanProjectClose;
  e.m_bProjectCanClose = true;
  s_Requests.Broadcast(e);

  return e.m_bProjectCanClose;
}

ezStatus ezEditorProject::CreateOrOpenProject(const char* szProjectPath, bool bCreate)
{
  CloseProject();

  new ezEditorProject(szProjectPath);

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

ezStatus ezEditorProject::OpenProject(const char* szProjectPath)
{
  ezStatus status = CreateOrOpenProject(szProjectPath, false);

  if (status.m_Result.Succeeded())
  {
    ezEditorEngineProcessConnection::GetInstance()->RestartProcess();
    ezEditorEngineProcessConnection::GetInstance()->WaitForMessage(ezGetStaticRTTI<ezProjectReadyMsgToEditor>());
  }

  return status;
}

ezStatus ezEditorProject::CreateProject(const char* szProjectPath)
{
  return CreateOrOpenProject(szProjectPath, true);
}

bool ezEditorProject::IsDocumentInProject(const char* szDocumentPath, ezString* out_RelativePath) const
{
  ezStringBuilder sProjectFolder = m_sProjectPath;
  sProjectFolder.PathParentDirectory();

  ezStringBuilder s = szDocumentPath;
  if (!s.IsPathBelowFolder(sProjectFolder))
    return false;

  if (out_RelativePath)
  {
    ezInt32 iTrimStart = sProjectFolder.GetCharacterCount();

    ezStringBuilder sText = szDocumentPath;
    sText.Shrink(iTrimStart, 0);

    *out_RelativePath = sText;
  }

  return true;
}

ezString ezEditorProject::FindProjectForDocument(const char* szDocumentPath)
{
#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)
  ezStringBuilder sPath = szDocumentPath;
  sPath.PathParentDirectory();

  ezStringBuilder sTemp;

  while (!sPath.IsEmpty())
  {
    sTemp = sPath;
    sTemp.AppendPath("*.project");

    ezFileSystemIterator it;
    if (it.StartSearch(sTemp, false, false).Succeeded())
    {
      ezStringBuilder sProjectPath = it.GetCurrentPath();
      sProjectPath.AppendPath(it.GetStats().m_sFileName);

      return sProjectPath;
    }

    sPath.PathParentDirectory();
  }
#endif

  return "";
}

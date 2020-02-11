#include <ToolsFoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Project/ToolsProject.h>

EZ_IMPLEMENT_SINGLETON(ezApplicationServices);

static ezApplicationServices g_instance;

ezApplicationServices::ezApplicationServices()
    : m_SingletonRegistrar(this)
{
}

void ezApplicationServices::SetApplicationName(const char* szName)
{
  m_sApplicationName = szName;
}

const char* ezApplicationServices::GetApplicationName() const
{
  return m_sApplicationName;
}


ezString ezApplicationServices::GetApplicationUserDataFolder() const
{
  ezStringBuilder path = ezOSFile::GetUserDataFolder();
  path.AppendPath("ezEngine Project", m_sApplicationName);
  path.MakeCleanPath();

  return path;
}

ezString ezApplicationServices::GetApplicationDataFolder() const
{
  ezStringBuilder sAppDir = ezOSFile::GetApplicationDirectory();
  sAppDir.AppendPath("../../../Data/Tools", m_sApplicationName);
  sAppDir.MakeCleanPath();

  return sAppDir;
}

ezString ezApplicationServices::GetApplicationPreferencesFolder() const
{
  return GetApplicationUserDataFolder();
}

ezString ezApplicationServices::GetProjectPreferencesFolder() const
{
  ezStringBuilder path = GetApplicationUserDataFolder();

  ezStringBuilder ProjectName = ezToolsProject::GetSingleton()->GetProjectDirectory();

  ezStringBuilder ProjectPath = ProjectName;
  ProjectPath.PathParentDirectory();

  const ezUInt32 uiPathHash = ezTempHashedString::ComputeHash(ProjectPath.GetData());

  ProjectName = ProjectName.GetFileName();

  path.AppendFormat("/Projects/{}_{}", uiPathHash, ProjectName);

  path.MakeCleanPath();
  return path;
}

ezString ezApplicationServices::GetDocumentPreferencesFolder(const ezDocument* pDocument) const
{
  ezStringBuilder path = GetProjectPreferencesFolder();

  ezStringBuilder sGuid;
  ezConversionUtils::ToString(pDocument->GetGuid(), sGuid);

  path.AppendPath(sGuid);

  path.MakeCleanPath();
  return path;
}

ezString ezApplicationServices::GetPrecompiledToolsFolder(bool bUsePrecompiledTools) const
{
  ezStringBuilder sPath = ezOSFile::GetApplicationDirectory();

  if (bUsePrecompiledTools)
  {
    sPath.AppendPath("../../../Data/Tools/Precompiled");
  }

  sPath.MakeCleanPath();

  return sPath;
}

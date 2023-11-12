#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
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

ezString ezApplicationServices::GetApplicationUserDataFolder() const
{
  ezStringBuilder path = ezOSFile::GetUserDataFolder();
  path.AppendPath("ezEngine Project", ezApplication::GetApplicationInstance()->GetApplicationName());
  path.MakeCleanPath();

  return path;
}

ezString ezApplicationServices::GetApplicationDataFolder() const
{
  ezStringBuilder sAppDir(">sdk/Data/Tools/", ezApplication::GetApplicationInstance()->GetApplicationName());

  ezStringBuilder result;
  ezFileSystem::ResolveSpecialDirectory(sAppDir, result).IgnoreResult();
  result.MakeCleanPath();

  return result;
}

ezString ezApplicationServices::GetApplicationPreferencesFolder() const
{
  return GetApplicationUserDataFolder();
}

ezString ezApplicationServices::GetProjectPreferencesFolder() const
{
  return GetProjectPreferencesFolder(ezToolsProject::GetSingleton()->GetProjectDirectory());
}

ezString ezApplicationServices::GetProjectPreferencesFolder(ezStringView sProjectFilePath) const
{
  ezStringBuilder path = GetApplicationUserDataFolder();

  sProjectFilePath.TrimWordEnd("ezProject");
  sProjectFilePath.TrimWordEnd("ezRemoteProject");
  sProjectFilePath.Trim("/\\");

  ezStringBuilder ProjectName = sProjectFilePath;

  ezStringBuilder ProjectPath = ProjectName;
  ProjectPath.PathParentDirectory();

  const ezUInt64 uiPathHash = ezHashingUtils::StringHash(ProjectPath.GetData());

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

ezString ezApplicationServices::GetSampleProjectsFolder() const
{
  ezStringBuilder sPath = ezOSFile::GetApplicationDirectory();

  sPath.AppendPath("../../../Data/Samples");

  sPath.MakeCleanPath();

  return sPath;
}

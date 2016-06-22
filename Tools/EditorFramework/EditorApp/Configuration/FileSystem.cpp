#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

ezString ezQtEditorApp::FindFolderWithSubPath(const char* szStartDirectory, const char* szSubPath) const
{
  ezStringBuilder CurStartPath = szStartDirectory;
  CurStartPath.MakeCleanPath();

  ezStringBuilder FullPath;
  
  while (!CurStartPath.IsEmpty())
  {
    FullPath = CurStartPath;
    FullPath.AppendPath(szSubPath);
    FullPath.MakeCleanPath();

    if (ezOSFile::ExistsDirectory(FullPath) || ezOSFile::ExistsFile(FullPath))
      return CurStartPath;

    CurStartPath.PathParentDirectory();
  }

  return ezString();
}

void ezQtEditorApp::AddPluginDataDirDependency(const char* szRelativePath, const char* szRootName)
{
  ezStringBuilder sPath = szRelativePath;
  sPath.MakeCleanPath();

  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    if (dd.m_sRelativePath == sPath)
    {
      dd.m_bHardCodedDependency = true;
      return;
    }
  }

  ezApplicationFileSystemConfig::DataDirConfig cfg;
  cfg.m_sRelativePath = sPath;
  cfg.m_bWritable = false;
  cfg.m_sRootName = szRootName;
  cfg.m_bHardCodedDependency = true;

  m_FileSystemConfig.m_DataDirs.PushBack(cfg);
}

void ezQtEditorApp::SetFileSystemConfig(const ezApplicationFileSystemConfig& cfg)
{
  if (m_FileSystemConfig == cfg)
    return;

  m_FileSystemConfig = cfg;
  ezQtEditorApp::GetSingleton()->AddReloadProjectRequiredReason("The data directory configuration has changed.");

  m_FileSystemConfig.CreateDataDirStubFiles();
}

void ezQtEditorApp::SetupDataDirectories()
{
  ezStringBuilder sPath = ezToolsProject::GetSingleton()->GetProjectFile();
  sPath.PathParentDirectory();

  ezApplicationConfig::SetProjectDirectory(sPath);
  m_FileSystemConfig.Load();

  ezEditorAppEvent e;
  e.m_pSender = this;
  e.m_Type = ezEditorAppEvent::Type::BeforeApplyDataDirectories;
  m_Events.Broadcast(e);

  ezStringBuilder sBaseDir = ezQtEditorApp::GetSingleton()->FindFolderWithSubPath(ezToolsProject::GetSingleton()->GetProjectDirectory(), "Data/Base");

  if (!sBaseDir.IsEmpty())
  {
    sBaseDir.AppendPath("Data/Base");
    sBaseDir.MakeRelativeTo(ezToolsProject::GetSingleton()->GetProjectDirectory());

    ezQtEditorApp::GetSingleton()->AddPluginDataDirDependency(sBaseDir, "base");
  }

  // Make sure the project directory is always in the list of data directories
  {
    bool bHasProjectDirMounted = false;
    for (auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      if (dd.m_sRelativePath.IsEmpty())
      {
        dd.m_bHardCodedDependency = true;
        bHasProjectDirMounted = true;
        break;
      }
    }

    if (!bHasProjectDirMounted)
    {
      ezApplicationFileSystemConfig::DataDirConfig dd;
      dd.m_bWritable = true;
      dd.m_sRootName = "project";
      dd.m_bHardCodedDependency = true;
      m_FileSystemConfig.m_DataDirs.PushBack(dd);
    }
  }

  // Tell the tools project that all data directories are ok to put documents in
  {
    for (const auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      sPath = ezApplicationConfig::GetProjectDirectory();
      sPath.AppendPath(dd.m_sRelativePath);
      ezToolsProject::GetSingleton()->AddAllowedDocumentRoot(sPath);
    }
  }

  m_FileSystemConfig.Apply();
}

bool ezQtEditorApp::MakeDataDirectoryRelativePathAbsolute(ezString & sPath) const
{
  if (ezPathUtils::IsAbsolutePath(sPath))
    return true;

  if (ezConversionUtils::IsStringUuid(sPath))
  {
    ezUuid guid = ezConversionUtils::ConvertStringToUuid(sPath);
    auto pAsset = ezAssetCurator::GetSingleton()->GetAssetInfo(guid);

    if (!pAsset)
      return false;

    sPath = pAsset->m_sAbsolutePath;
    return true;
  }

  ezStringBuilder sTemp;

  for (ezUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    sTemp = m_FileSystemConfig.GetProjectDirectory();
    sTemp.AppendPath(dd.m_sRelativePath, sPath);
    sTemp.MakeCleanPath();

    if (ezOSFile::ExistsFile(sTemp) || ezOSFile::ExistsDirectory(sTemp))
    {
      sPath = sTemp;
      return true;
    }
  }

  if (!m_FileSystemConfig.m_DataDirs.IsEmpty())
  {
    sTemp = m_FileSystemConfig.GetProjectDirectory();
    sTemp.AppendPath(m_FileSystemConfig.m_DataDirs[0].m_sRelativePath, sPath);
    sTemp.MakeCleanPath();
  }

  return false;
}

bool ezQtEditorApp::MakePathDataDirectoryRelative(ezString & sPath) const
{
  ezStringBuilder sTemp;
  ezStringBuilder sResult = sPath;

  for (ezUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    sTemp = m_FileSystemConfig.GetProjectDirectory();
    sTemp.AppendPath(dd.m_sRelativePath);

    if (sResult.IsPathBelowFolder(sTemp))
    {
      sResult.MakeRelativeTo(sTemp);
      sPath = sResult;
      return true;
    }
  }

  sResult.MakeRelativeTo(m_FileSystemConfig.GetProjectDirectory());
  sPath = sResult;

  return false;
}


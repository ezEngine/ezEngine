#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

void ezQtEditorApp::AddPluginDataDirDependency(const char* szSdkRootRelativePath, const char* szRootName, bool bWriteable)
{
  ezStringBuilder sPath = szSdkRootRelativePath;
  sPath.MakeCleanPath();

  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    if (dd.m_sSdkRootRelativePath == sPath)
    {
      dd.m_bHardCodedDependency = true;

      if (bWriteable)
        dd.m_bWritable = true;

      return;
    }
  }

  ezApplicationFileSystemConfig::DataDirConfig cfg;
  cfg.m_sSdkRootRelativePath = sPath;
  cfg.m_bWritable = bWriteable;
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
  ezApplicationConfig::DetectSdkRootDirectory();

  ezStringBuilder sPath = ezToolsProject::GetSingleton()->GetProjectFile();
  sPath.PathParentDirectory();

  ezApplicationConfig::SetProjectDirectory(sPath);
  m_FileSystemConfig.Load();

  ezEditorAppEvent e;
  e.m_pSender = this;
  e.m_Type = ezEditorAppEvent::Type::BeforeApplyDataDirectories;
  m_Events.Broadcast(e);

  ezStringBuilder sBaseDir;
  ezQtEditorApp::GetSingleton()->AddPluginDataDirDependency("Data/Base", "base", false);

  ezStringBuilder sProjectInSdk = sPath;
  sProjectInSdk.MakeRelativeTo(ezApplicationConfig::GetSdkRootDirectory());

  ezQtEditorApp::GetSingleton()->AddPluginDataDirDependency(sProjectInSdk, "project", true);

  // Tell the tools project that all data directories are ok to put documents in
  {
    for (const auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      sPath = ezApplicationConfig::GetSdkRootDirectory();
      sPath.AppendPath(dd.m_sSdkRootRelativePath);
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
    auto pAsset = ezAssetCurator::GetSingleton()->GetAssetInfo2(guid);

    if (!pAsset)
      return false;

    sPath = pAsset->m_sAbsolutePath;
    return true;
  }

  ezStringBuilder sTemp;

  for (ezUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    sTemp = ezApplicationConfig::GetSdkRootDirectory();
    sTemp.AppendPath(dd.m_sSdkRootRelativePath, sPath);
    sTemp.MakeCleanPath();

    if (ezOSFile::ExistsFile(sTemp) || ezOSFile::ExistsDirectory(sTemp))
    {
      sPath = sTemp;
      return true;
    }
  }

  if (!m_FileSystemConfig.m_DataDirs.IsEmpty())
  {
    sTemp = ezApplicationConfig::GetSdkRootDirectory();
    sTemp.AppendPath(m_FileSystemConfig.m_DataDirs[0].m_sSdkRootRelativePath, sPath);
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

    sTemp = ezApplicationConfig::GetSdkRootDirectory();
    sTemp.AppendPath(dd.m_sSdkRootRelativePath);

    if (sResult.IsPathBelowFolder(sTemp))
    {
      sResult.MakeRelativeTo(sTemp);
      sPath = sResult;
      return true;
    }
  }

  sResult.MakeRelativeTo(ezApplicationConfig::GetSdkRootDirectory());
  sPath = sResult;

  return false;
}


#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>
#include <EditorFramework/Assets/AssetCurator.h>

void ezQtEditorApp::AddPluginDataDirDependency(const char* szSdkRootRelativePath, const char* szRootName, bool bWriteable)
{
  ezStringBuilder sPath = szSdkRootRelativePath;
  sPath.MakeCleanPath();

  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    if (dd.m_sDataDirSpecialPath == sPath)
    {
      dd.m_bHardCodedDependency = true;

      if (bWriteable)
        dd.m_bWritable = true;

      return;
    }
  }

  ezApplicationFileSystemConfig::DataDirConfig cfg;
  cfg.m_sDataDirSpecialPath = sPath;
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
  ezFileSystem::DetectSdkRootDirectory();

  ezStringBuilder sPath = ezToolsProject::GetSingleton()->GetProjectDirectory();

  ezFileSystem::SetSpecialDirectory("project", sPath);

  sPath.AppendPath("DataDirectories.ddl");
  // we cannot use the default ":project/" path here, because that data directory will only be configured a few lines below
  // so instead we use the absolute path directly
  m_FileSystemConfig.Load(sPath);

  ezEditorAppEvent e;
  e.m_Type = ezEditorAppEvent::Type::BeforeApplyDataDirectories;
  m_Events.Broadcast(e);

  ezQtEditorApp::GetSingleton()->AddPluginDataDirDependency(">sdk/Data/Base", "base", false);
  ezQtEditorApp::GetSingleton()->AddPluginDataDirDependency(">project/", "project", true);

  // Tell the tools project that all data directories are ok to put documents in
  {
    for (const auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      if (ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sPath).Succeeded())
      {
        ezToolsProject::GetSingleton()->AddAllowedDocumentRoot(sPath);
      }
    }
  }

  m_FileSystemConfig.Apply();
}

bool ezQtEditorApp::MakeParentDataDirectoryRelativePathAbsolute(ezStringBuilder& sPath) const
{
  if (ezPathUtils::IsAbsolutePath(sPath))
    return true;

  if (ezPathUtils::IsRootedPath(sPath))
  {
    ezStringBuilder sAbsPath;
    if (ezFileSystem::ResolvePath(sPath, &sAbsPath, nullptr).Succeeded())
    {
      sPath = sAbsPath;
      return true;
    }

    return false;
  }

  if (ezConversionUtils::IsStringUuid(sPath))
  {
    ezUuid guid = ezConversionUtils::ConvertStringToUuid(sPath);
    auto pAsset = ezAssetCurator::GetSingleton()->GetSubAsset(guid);

    if (pAsset == nullptr)
      return false;

    sPath = pAsset->m_pAssetInfo->m_sAbsolutePath;
    return true;
  }

  ezStringBuilder sTemp;

  for (ezUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    sTemp.PathParentDirectory(); // the secret sauce is here
    sTemp.AppendPath(sPath);
    sTemp.MakeCleanPath();

    if (ezOSFile::ExistsFile(sTemp) || ezOSFile::ExistsDirectory(sTemp))
    {
      sPath = sTemp;
      return true;
    }
  }

  return false;

}

bool ezQtEditorApp::MakeDataDirectoryRelativePathAbsolute(ezStringBuilder& sPath) const
{
  if (ezPathUtils::IsAbsolutePath(sPath))
    return true;

  if (ezPathUtils::IsRootedPath(sPath))
  {
    ezStringBuilder sAbsPath;
    if (ezFileSystem::ResolvePath(sPath, &sAbsPath, nullptr).Succeeded())
    {
      sPath = sAbsPath;
      return true;
    }

    return false;
  }

  if (ezConversionUtils::IsStringUuid(sPath))
  {
    ezUuid guid = ezConversionUtils::ConvertStringToUuid(sPath);
    auto pAsset = ezAssetCurator::GetSingleton()->GetSubAsset(guid);

    if (pAsset == nullptr)
      return false;

    sPath = pAsset->m_pAssetInfo->m_sAbsolutePath;
    return true;
  }

  ezStringBuilder sTemp;

  for (ezUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    sTemp.AppendPath(sPath);
    sTemp.MakeCleanPath();

    if (ezOSFile::ExistsFile(sTemp) || ezOSFile::ExistsDirectory(sTemp))
    {
      sPath = sTemp;
      return true;
    }
  }

  return false;
}

bool ezQtEditorApp::MakeDataDirectoryRelativePathAbsolute(ezString& sPath) const
{
  ezStringBuilder sTemp = sPath;
  bool bRes = MakeDataDirectoryRelativePathAbsolute(sTemp);
  sPath = sTemp;
  return bRes;
}

bool ezQtEditorApp::MakePathDataDirectoryRelative(ezStringBuilder& sPath) const
{
  ezStringBuilder sTemp;

  for (ezUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    if (sPath.IsPathBelowFolder(sTemp))
    {
      sPath.MakeRelativeTo(sTemp);
      return true;
    }
  }

  sPath.MakeRelativeTo(ezFileSystem::GetSdkRootDirectory());
  return false;
}

bool ezQtEditorApp::MakePathDataDirectoryParentRelative(ezStringBuilder& sPath) const
{
  ezStringBuilder sTemp;

  for (ezUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    if (sPath.IsPathBelowFolder(sTemp))
    {
      sTemp.PathParentDirectory();

      sPath.MakeRelativeTo(sTemp);
      return true;
    }
  }

  sPath.MakeRelativeTo(ezFileSystem::GetSdkRootDirectory());
  return false;
}

bool ezQtEditorApp::MakePathDataDirectoryRelative(ezString& sPath) const
{
  ezStringBuilder sTemp = sPath;
  bool bRes = MakePathDataDirectoryRelative(sTemp);
  sPath = sTemp;
  return bRes;
}



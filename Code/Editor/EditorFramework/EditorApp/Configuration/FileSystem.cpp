#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

void ezQtEditorApp::AddPluginDataDirDependency(const char* szSdkRootRelativePath, const char* szRootName, bool bWriteable, ezUInt32 uiInsertIndex)
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

  if (uiInsertIndex == ezInvalidIndex || uiInsertIndex >= m_FileSystemConfig.m_DataDirs.GetCount())
    m_FileSystemConfig.m_DataDirs.PushBack(cfg);
  else
    m_FileSystemConfig.m_DataDirs.InsertAt(uiInsertIndex, cfg);
}

void ezQtEditorApp::SetFileSystemConfig(const ezApplicationFileSystemConfig& cfg)
{
  if (m_FileSystemConfig == cfg)
    return;

  m_FileSystemConfig = cfg;
  m_FileSystemConfig.CreateDataDirStubFiles().IgnoreResult();
  m_FileSystemConfig.Save().IgnoreResult();

  ezQtEditorApp::GetSingleton()->AddReloadProjectRequiredReason("The data directory configuration has changed.");
}

void ezQtEditorApp::SetupDataDirectories()
{
  EZ_PROFILE_SCOPE("SetupDataDirectories");
  ezFileSystem::DetectSdkRootDirectory().IgnoreResult();

  ezStringBuilder sPath = ezToolsProject::GetSingleton()->GetProjectDirectory();

  ezFileSystem::SetSpecialDirectory("project", sPath);

  sPath.AppendPath("RuntimeConfigs/DataDirectories.ddl");
  // we cannot use the default ":project/" path here, because that data directory will only be configured a few lines below
  // so instead we use the absolute path directly
  m_FileSystemConfig.Load(sPath);

  ezEditorAppEvent e;
  e.m_Type = ezEditorAppEvent::Type::BeforeApplyDataDirectories;
  m_Events.Broadcast(e);

  ezQtEditorApp::GetSingleton()->AddPluginDataDirDependency(">sdk/Data/Base", "base", false);

  // Remove stale plugin-bundle data directories (bundle no longer active, or the legacy shared mount) before re-injecting
  // the ones that are currently active. This self-heals when a plugin is disabled.
  {
    ezSet<ezString> knownBundleDirs;
    ezQtEditorApp::GetSingleton()->GetAllKnownBundleDataDirectories(knownBundleDirs);

    ezStringBuilder sLegacy = ">sdk/Data/Plugins";
    sLegacy.MakeCleanPath();
    knownBundleDirs.Insert(sLegacy);

    for (ezUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
    {
      if (knownBundleDirs.Contains(m_FileSystemConfig.m_DataDirs[i - 1].m_sDataDirSpecialPath))
      {
        m_FileSystemConfig.m_DataDirs.RemoveAtAndCopy(i - 1);
      }
    }
  }

  // Inject the data directories declared by all currently active plugin bundles (mandatory/selected + transitive
  // requirements). These have to end up before ">project/", because data directories are searched back to front:
  // whatever is listed after the project shadows the project's own files, and a project must be able to override
  // plugin provided files (e.g. ship its own version of a particle shader).
  // Appending is not enough for that, because the project directory is usually already part of the config that was
  // just loaded, so the insert position has to be looked up.
  {
    ezSet<ezString> activeBundleDirs;
    ezQtEditorApp::GetSingleton()->GetActiveBundleDataDirectories(activeBundleDirs);

    ezStringBuilder sProjectDir = ">project/";
    sProjectDir.MakeCleanPath();

    ezUInt32 uiInsertIndex = ezInvalidIndex;
    for (ezUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
    {
      if (m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath == sProjectDir)
      {
        uiInsertIndex = i;
        break;
      }
    }

    for (const ezString& sDir : activeBundleDirs)
    {
      ezQtEditorApp::GetSingleton()->AddPluginDataDirDependency(sDir, nullptr, false, uiInsertIndex);

      // the loop above removed all bundle directories, so each of these really is an insert
      if (uiInsertIndex != ezInvalidIndex)
        ++uiInsertIndex;
    }
  }

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

bool ezQtEditorApp::MakeParentDataDirectoryRelativePathAbsolute(ezStringBuilder& ref_sPath, bool bCheckExists) const
{
  ref_sPath.MakeCleanPath();

  if (ezPathUtils::IsAbsolutePath(ref_sPath))
    return true;

  if (ezPathUtils::IsRootedPath(ref_sPath))
  {
    ezStringBuilder sAbsPath;
    if (ezFileSystem::ResolvePath(ref_sPath, &sAbsPath, nullptr).Succeeded())
    {
      ref_sPath = sAbsPath;
      return true;
    }

    return false;
  }

  if (ezConversionUtils::IsStringUuid(ref_sPath))
  {
    ezUuid guid = ezConversionUtils::ConvertStringToUuid(ref_sPath);
    auto pAsset = ezAssetCurator::GetSingleton()->GetSubAsset(guid);

    // m_pAssetInfo is null for a sub-asset the curator knows but holds no file information for, so it
    // has to be checked separately from the asset itself.
    if (pAsset == nullptr || pAsset->m_pAssetInfo == nullptr)
      return false;

    ref_sPath = pAsset->m_pAssetInfo->m_Path;
    return true;
  }

  ezStringBuilder sTemp, sFolder, sDataDirName;

  const char* szEnd = ref_sPath.FindSubString("/");
  if (szEnd)
  {
    sDataDirName.SetSubString_FromTo(ref_sPath.GetData(), szEnd);
  }
  else
  {
    sDataDirName = ref_sPath;
  }

  for (ezUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    // only check data directories that start with the required name
    while (sTemp.EndsWith("/") || sTemp.EndsWith("\\"))
      sTemp.Shrink(0, 1);
    const ezStringView folderName = sTemp.GetFileName();

    if (sDataDirName != folderName)
      continue;

    sTemp.PathParentDirectory(); // the secret sauce is here
    sTemp.AppendPath(ref_sPath);
    sTemp.MakeCleanPath();

    if (!bCheckExists || ezOSFile::ExistsFile(sTemp) || ezOSFile::ExistsDirectory(sTemp))
    {
      ref_sPath = sTemp;
      return true;
    }
  }

  return false;
}

bool ezQtEditorApp::MakeDataDirectoryRelativePathAbsolute(ezStringBuilder& ref_sPath) const
{
  if (ezPathUtils::IsAbsolutePath(ref_sPath))
    return true;

  if (ezPathUtils::IsRootedPath(ref_sPath))
  {
    ezStringBuilder sAbsPath;
    if (ezFileSystem::ResolvePath(ref_sPath, &sAbsPath, nullptr).Succeeded())
    {
      ref_sPath = sAbsPath;
      return true;
    }

    return false;
  }

  if (ezConversionUtils::IsStringUuid(ref_sPath))
  {
    ezUuid guid = ezConversionUtils::ConvertStringToUuid(ref_sPath);
    auto pAsset = ezAssetCurator::GetSingleton()->GetSubAsset(guid);

    // m_pAssetInfo is null for a sub-asset the curator knows but holds no file information for, so it
    // has to be checked separately from the asset itself.
    if (pAsset == nullptr || pAsset->m_pAssetInfo == nullptr)
      return false;

    ref_sPath = pAsset->m_pAssetInfo->m_Path;
    return true;
  }

  ezStringBuilder sTemp;

  for (ezUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    sTemp.AppendPath(ref_sPath);
    sTemp.MakeCleanPath();

    if (ezOSFile::ExistsFile(sTemp) || ezOSFile::ExistsDirectory(sTemp))
    {
      ref_sPath = sTemp;
      return true;
    }
  }

  return false;
}

bool ezQtEditorApp::MakeDataDirectoryRelativePathAbsolute(ezString& ref_sPath) const
{
  ezStringBuilder sTemp = ref_sPath;
  bool bRes = MakeDataDirectoryRelativePathAbsolute(sTemp);
  ref_sPath = sTemp;
  return bRes;
}

bool ezQtEditorApp::MakePathDataDirectoryRelative(ezStringBuilder& ref_sPath) const
{
  ezStringBuilder sTemp;

  for (ezUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    if (ref_sPath.IsPathBelowFolder(sTemp))
    {
      ref_sPath.MakeRelativeTo(sTemp).IgnoreResult();
      return true;
    }
  }

  ref_sPath.MakeRelativeTo(ezFileSystem::GetSdkRootDirectory()).IgnoreResult();
  return false;
}

bool ezQtEditorApp::MakePathDataDirectoryParentRelative(ezStringBuilder& ref_sPath) const
{
  ezStringBuilder sTemp;

  for (ezUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    if (ref_sPath.IsPathBelowFolder(sTemp))
    {
      sTemp.PathParentDirectory();

      ref_sPath.MakeRelativeTo(sTemp).IgnoreResult();
      return true;
    }
  }

  ref_sPath.MakeRelativeTo(ezFileSystem::GetSdkRootDirectory()).IgnoreResult();
  return false;
}

bool ezQtEditorApp::MakePathDataDirectoryRelative(ezString& ref_sPath) const
{
  ezStringBuilder sTemp = ref_sPath;
  bool bRes = MakePathDataDirectoryRelative(sTemp);
  ref_sPath = sTemp;
  return bRes;
}

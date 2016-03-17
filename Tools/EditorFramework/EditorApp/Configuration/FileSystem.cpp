#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

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

  // Make sure the project directory is always in the list of data directories
  {
    bool bHasProjectDirMounted = false;
    for (const auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      if (dd.m_sRelativePath.IsEmpty())
      {
        bHasProjectDirMounted = true;
        break;
      }
    }

    if (!bHasProjectDirMounted)
    {
      ezApplicationFileSystemConfig::DataDirConfig dd;
      dd.m_bWritable = true;
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
}

bool ezQtEditorApp::MakeDataDirectoryRelativePathAbsolute(ezString & sPath) const
{
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


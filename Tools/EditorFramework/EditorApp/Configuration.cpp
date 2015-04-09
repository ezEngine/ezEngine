#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

void ezEditorApp::SetFileSystemConfig(const ezApplicationFileSystemConfig& cfg)
{
  if (m_FileSystemConfig == cfg)
    return;

  m_FileSystemConfig = cfg;
  ezEditorApp::GetInstance()->AddReloadProjectRequiredReason("The data directory configuration has changed.");

  m_FileSystemConfig.CreateDataDirStubFiles();
}

void ezEditorApp::SetEnginePluginConfig(const ezApplicationPluginConfig& cfg)
{
  m_EnginePluginConfig = cfg;
}

void ezEditorApp::SetupDataDirectories()
{
  ezStringBuilder sPath = ezToolsProject::GetInstance()->GetProjectPath();
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
      ezToolsProject::GetInstance()->AddAllowedDocumentRoot(sPath);
    }
  }
}
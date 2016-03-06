#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>
#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>

void ezQtEditorApp::SetFileSystemConfig(const ezApplicationFileSystemConfig& cfg)
{
  if (m_FileSystemConfig == cfg)
    return;

  m_FileSystemConfig = cfg;
  ezQtEditorApp::GetInstance()->AddReloadProjectRequiredReason("The data directory configuration has changed.");

  m_FileSystemConfig.CreateDataDirStubFiles();
}

bool ezQtEditorApp::MakeDataDirectoryRelativePathAbsolute(ezString & sPath) const
{
  if (ezConversionUtils::IsStringUuid(sPath))
  {
    ezUuid guid = ezConversionUtils::ConvertStringToUuid(sPath);
    auto pAsset = ezAssetCurator::GetInstance()->GetAssetInfo(guid);

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

ezStatus ezQtEditorApp::SaveTagRegistry()
{
  EZ_LOG_BLOCK("ezQtEditorApp::SaveTagRegistry()");

  ezStringBuilder sPath;
  sPath = ezApplicationConfig::GetProjectDirectory();
  sPath.AppendPath("Tags.ezManifest");

  ezFileWriter file;
  if (file.Open(sPath).Failed())
  {
    return ezStatus("Could not open tags config file '%s' for writing", sPath.GetData());
  }

  ezToolsTagRegistry::WriteToJSON(file);
  return ezStatus(EZ_SUCCESS);
}

void ezQtEditorApp::ReadTagRegistry()
{
  EZ_LOG_BLOCK("ezQtEditorApp::ReadTagRegistry()");

  ezToolsTagRegistry::Clear();

  ezStringBuilder sPath;
  sPath = ezApplicationConfig::GetProjectDirectory();
  sPath.AppendPath("Tags.ezManifest");

  ezFileReader file;
  if (file.Open(sPath).Failed())
  {
    ezLog::Warning("Could not open tags config file '%s'", sPath.GetData());
    
    ezStatus res = SaveTagRegistry();
    if (res.m_Result.Failed())
    {
      ezLog::Error("%s", res.m_sMessage.GetData());
    }
  }
  else
  {
    ezStatus res = ezToolsTagRegistry::ReadFromJSON(file);
    if (res.m_Result.Failed())
    {
      ezLog::Error("%s", res.m_sMessage.GetData());
    }
  }


  // TODO: Add default tags
  ezToolsTag tag;
  tag.m_sName = "EditorHidden";
  tag.m_sCategory = "Default";
  ezToolsTagRegistry::AddTag(tag);
}

void ezQtEditorApp::SetupDataDirectories()
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

void ezQtEditorApp::ReadEnginePluginConfig()
{
  m_EnginePluginConfig.Load();

  // remove all plugin dependencies that are stored in file
  for (ezUInt32 i = 0; i < m_EnginePluginConfig.m_Plugins.GetCount(); ++i)
  {
    const bool bManual = m_EnginePluginConfig.m_Plugins[i].m_sDependecyOf.Contains("<manual>");

    m_EnginePluginConfig.m_Plugins[i].m_sDependecyOf.Clear();

    if (bManual)
      m_EnginePluginConfig.m_Plugins[i].m_sDependecyOf.Insert("<manual>");
  }

  // and add all dependencies that we have now
  for (auto eplug = m_AdditionalRuntimePluginDependencies.GetIterator(); eplug.IsValid(); ++eplug)
  {
    for (auto rdep = eplug.Value().GetIterator(); rdep.IsValid(); ++rdep)
    {
      ezApplicationPluginConfig::PluginConfig cfg;
      cfg.m_sDependecyOf.Insert(eplug.Key());
      cfg.m_sRelativePath = *rdep;

      m_EnginePluginConfig.AddPlugin(cfg);
    }
  }

  ezUInt32 count = m_EnginePluginConfig.m_Plugins.GetCount();
  for (ezUInt32 i = 0; i < count; )
  {
    if (m_EnginePluginConfig.m_Plugins[i].m_sDependecyOf.IsEmpty())
    {
      m_EnginePluginConfig.m_Plugins.RemoveAt(i);
      --count;
    }
    else
      ++i;
  }

  // save new state again
  m_EnginePluginConfig.Save();

  DetectAvailableEnginePlugins();
}

void ezQtEditorApp::AddRuntimePluginDependency(const char* szEditorPluginName, const char* szRuntimeDependency)
{
  m_AdditionalRuntimePluginDependencies[szEditorPluginName].Insert(szRuntimeDependency);
}



void ezQtEditorApp::CreatePanels()
{
  new ezQtLogPanel();
  new ezQtAssetBrowserPanel();
}



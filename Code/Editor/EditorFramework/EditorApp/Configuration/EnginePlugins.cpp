#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>

void ezQtEditorApp::DetectAvailableEnginePlugins()
{
  // TODO plugins

  StoreEnginePluginModificationTimes();
}

void ezQtEditorApp::StoreEnginePluginModificationTimes()
{
  // TODO plugins

  //for (const auto& plugin : m_EnginePluginConfig.m_Plugins)
  //{
  //  auto& data = s_EnginePlugins.m_Plugins[plugin.m_sAppDirRelativePath];

  //  ezStringBuilder sPath, sCopy;
  //  ezPlugin::GetPluginPaths(plugin.m_sAppDirRelativePath, sPath, sCopy, 0);

  //  ezFileStats stats;
  //  if (ezOSFile::GetFileStats(sPath, stats).Succeeded())
  //  {
  //    data.m_LastModification = stats.m_LastModificationTime;
  //  }
  //}
}

bool ezQtEditorApp::CheckForEnginePluginModifications()
{
  // TODO plugins

  //for (const auto& plugin : m_EnginePluginConfig.m_Plugins)
  //{
  //  auto& data = s_EnginePlugins.m_Plugins[plugin.m_sAppDirRelativePath];
  //  // only plugins for which a copy was loaded can be modified in parallel
  //  if (!plugin.m_bLoadCopy)
  //    continue;

  //  ezStringBuilder sPath, sCopy;
  //  ezPlugin::GetPluginPaths(plugin.m_sAppDirRelativePath, sPath, sCopy, 0);

  //  ezFileStats stats;
  //  if (ezOSFile::GetFileStats(sPath, stats).Succeeded())
  //  {
  //    if (!data.m_LastModification.IsValid() || stats.m_LastModificationTime.Compare(data.m_LastModification, ezTimestamp::CompareMode::Newer))
  //    {
  //      return true;
  //    }
  //  }
  //}

  return false;
}

void ezQtEditorApp::RestartEngineProcessIfPluginsChanged()
{
  if (m_LastPluginModificationCheck + ezTime::Seconds(2) > ezTime::Now())
    return;

  m_LastPluginModificationCheck = ezTime::Now();

  for (auto pMan : ezDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDoc : pMan->ezDocumentManager::GetAllOpenDocuments())
    {
      if (!pDoc->CanEngineProcessBeRestarted())
      {
        // not allowed to restart at the moment
        return;
      }
    }
  }

  if (!CheckForEnginePluginModifications())
    return;

  ezLog::Info("Engine plugins have changed, restarting engine process.");

  StoreEnginePluginModificationTimes();
  ezEditorEngineProcessConnection::GetSingleton()->RestartProcess().IgnoreResult();
}

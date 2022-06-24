#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>

void ezQtEditorApp::StoreEnginePluginModificationTimes()
{
  for (auto it : m_PluginBundles.m_Plugins)
  {
    ezPluginBundle& plugin = it.Value();

    if (!plugin.m_bLoadCopy)
      continue;

    for (const ezString& rt : plugin.m_RuntimePlugins)
    {
      ezStringBuilder sPath, sCopy;
      ezPlugin::GetPluginPaths(rt, sPath, sCopy, 0);

      ezFileStats stats;
      if (ezOSFile::GetFileStats(sPath, stats).Succeeded())
      {
        if (!plugin.m_LastModificationTime.IsValid() || stats.m_LastModificationTime.Compare(plugin.m_LastModificationTime, ezTimestamp::CompareMode::Newer))
        {
          // store the maximum (latest) modification timestamp
          plugin.m_LastModificationTime = stats.m_LastModificationTime;
        }
      }
    }
  }
}

bool ezQtEditorApp::CheckForEnginePluginModifications()
{
  // TODO plugins

  // for (const auto& plugin : m_EnginePluginConfig.m_Plugins)
  //{
  //   auto& data = s_EnginePlugins.m_Plugins[plugin.m_sAppDirRelativePath];
  //   // only plugins for which a copy was loaded can be modified in parallel
  //   if (!plugin.m_bLoadCopy)
  //     continue;

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

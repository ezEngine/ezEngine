#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>

void ezQtEditorApp::StoreEnginePluginModificationTimes()
{
  for (auto it : m_PluginBundles.m_Plugins)
  {
    ezPluginBundle& plugin = it.Value();

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
  for (auto it : m_PluginBundles.m_Plugins)
  {
    ezPluginBundle& plugin = it.Value();

    if (plugin.m_bMissing)
    {
      DetectAvailablePluginBundles();
      break;
    }
  }

  for (auto it : m_PluginBundles.m_Plugins)
  {
    ezPluginBundle& plugin = it.Value();

    if (!plugin.m_bSelected || !plugin.m_bLoadCopy)
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
          return true;
        }
      }
    }
  }

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
  ezEditorEngineProcessConnection::GetSingleton()->SetPluginConfig(GetRuntimePluginConfig(true));
  ezEditorEngineProcessConnection::GetSingleton()->RestartProcess().IgnoreResult();
}

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>

void ezQtEditorApp::DetectAvailableEnginePlugins()
{
  s_EnginePlugins.m_Plugins.Clear();

#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)
  {
    ezStringBuilder sSearch = ezOSFile::GetApplicationDirectory();
    ;
    sSearch.AppendPath("*.dll");

    ezFileSystemIterator fsit;
    for (fsit.StartSearch(sSearch.GetData(), ezFileSystemIteratorFlags::ReportFiles); fsit.IsValid(); fsit.Next())
    {
      ezStringBuilder sPlugin = fsit.GetStats().m_sName;
      sPlugin.RemoveFileExtension();

      if (sPlugin.FindSubString_NoCase("EnginePlugin") != nullptr || sPlugin.EndsWith_NoCase("Plugin"))
      {
        s_EnginePlugins.m_Plugins[sPlugin].m_bAvailable = true;
      }
    }
  }
#endif

  for (const auto& plugin : m_EnginePluginConfig.m_Plugins)
  {
    s_EnginePlugins.m_Plugins[plugin.m_sAppDirRelativePath].m_bActive = true;
    s_EnginePlugins.m_Plugins[plugin.m_sAppDirRelativePath].m_bToBeLoaded = plugin.m_sDependecyOf.Contains("<manual>");
    s_EnginePlugins.m_Plugins[plugin.m_sAppDirRelativePath].m_bLoadCopy = plugin.m_bLoadCopy;
  }

  StoreEnginePluginModificationTimes();
}

void ezQtEditorApp::StoreEnginePluginModificationTimes()
{
  for (const auto& plugin : m_EnginePluginConfig.m_Plugins)
  {
    auto& data = s_EnginePlugins.m_Plugins[plugin.m_sAppDirRelativePath];

    ezStringBuilder sPath, sCopy;
    ezPlugin::GetPluginPaths(plugin.m_sAppDirRelativePath, sPath, sCopy, 0);

    ezFileStats stats;
    if (ezOSFile::GetFileStats(sPath, stats).Succeeded())
    {
      data.m_LastModification = stats.m_LastModificationTime;
    }
  }
}

bool ezQtEditorApp::CheckForEnginePluginModifications()
{
  for (const auto& plugin : m_EnginePluginConfig.m_Plugins)
  {
    auto& data = s_EnginePlugins.m_Plugins[plugin.m_sAppDirRelativePath];
    // only plugins for which a copy was loaded can be modified in parallel
    if (!plugin.m_bLoadCopy)
      continue;

    ezStringBuilder sPath, sCopy;
    ezPlugin::GetPluginPaths(plugin.m_sAppDirRelativePath, sPath, sCopy, 0);

    ezFileStats stats;
    if (ezOSFile::GetFileStats(sPath, stats).Succeeded())
    {
      if (stats.m_LastModificationTime.Compare(data.m_LastModification, ezTimestamp::CompareMode::Newer))
      {
        return true;
      }
    }
  }

  return false;
}

void ezQtEditorApp::StoreEnginePluginsToBeLoaded()
{
  bool bChange = false;

  for (auto it = s_EnginePlugins.m_Plugins.GetIterator(); it.IsValid(); ++it)
  {
    ezApplicationPluginConfig::PluginConfig cfg;
    cfg.m_sAppDirRelativePath = it.Key();
    bool bToBeLoaded = it.Value().m_bToBeLoaded;

    if (it.Value().m_bLoadCopy)
      cfg.m_bLoadCopy = true;

    // make sure these settings were not manually messed with
    if (cfg.m_sAppDirRelativePath.FindSubString_NoCase("EnginePlugin") != nullptr)
    {
      cfg.m_bLoadCopy = false;
      cfg.m_sDependecyOf.Remove("<manual>");
      bToBeLoaded = false;
    }

    if (bToBeLoaded)
    {
      bChange = m_EnginePluginConfig.AddPlugin(cfg) || bChange;
    }
    else
    {
      bChange = m_EnginePluginConfig.RemovePlugin(cfg) || bChange;
    }
  }

  if (bChange)
  {
    AddReloadProjectRequiredReason("The set of active engine plugins was changed.");
    m_EnginePluginConfig.Save().IgnoreResult();
  }
}


void ezQtEditorApp::ReadEnginePluginConfig()
{
  EZ_PROFILE_SCOPE("ReadEnginePluginConfig");
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
      cfg.m_sAppDirRelativePath = *rdep;

      m_EnginePluginConfig.AddPlugin(cfg);
    }
  }

  ezUInt32 count = m_EnginePluginConfig.m_Plugins.GetCount();
  for (ezUInt32 i = 0; i < count;)
  {
    if (m_EnginePluginConfig.m_Plugins[i].m_sDependecyOf.IsEmpty())
    {
      m_EnginePluginConfig.m_Plugins.RemoveAtAndCopy(i);
      --count;
    }
    else
      ++i;
  }

  // save new state again
  m_EnginePluginConfig.Save().IgnoreResult();

  DetectAvailableEnginePlugins();

  ValidateEnginePluginConfig();
}

void ezQtEditorApp::AddRuntimePluginDependency(const char* szEditorPluginName, const char* szRuntimeDependency)
{
  m_AdditionalRuntimePluginDependencies[szEditorPluginName].Insert(szRuntimeDependency);
}


void ezQtEditorApp::ValidateEnginePluginConfig()
{
  ezPluginSet& Plugins = ezQtEditorApp::GetSingleton()->GetEnginePlugins();

  ezStringBuilder sMissingPlugins;
  ezStringBuilder sIllformedPlugins;

  for (auto it = Plugins.m_Plugins.GetIterator(); it.IsValid(); ++it)
  {
    const auto& sName = it.Key();

    if (!sName.EndsWith_NoCase("plugin") && !sName.FindSubString_NoCase("EnginePlugin"))
    {
      sIllformedPlugins.Append(sName, "\n");
      ezLog::Warning("Ill-formed plugin name '{0}'. Engine plugin names should start with 'EnginePlugin' or end with 'Plugin'.", sName);
    }
    else if (!it.Value().m_bAvailable)
    {
      sMissingPlugins.Append(sName, "\n");
      ezLog::Error("Engine plugin is missing: '{0}'.", sName);
    }
  }

  if (!sMissingPlugins.IsEmpty() || !sIllformedPlugins.IsEmpty())
  {
    ezStringBuilder sMsg;
    sMsg.Format("There are errors in the engine plugin configuration.\n\n");

    if (!sMissingPlugins.IsEmpty())
    {
      sMsg.AppendFormat("Missing engine plugins:\n{0}\n", sMissingPlugins);
    }

    if (!sIllformedPlugins.IsEmpty())
    {
      sMsg.AppendFormat("Plugins that do not conform to the expected naming scheme:\n{0}\nPure runtime plugins should use the suffix "
                        "'Plugin'.\nPlugins that implement editor functionality but need to run on the engine side should use the prefix "
                        "'EnginePlugin'.",
        sIllformedPlugins);
    }

    ezQtUiServices::MessageBoxWarning(sMsg);
  }
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

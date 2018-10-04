#include <PCH.h>

#include <EditorFramework/EditorApp/Configuration/Plugins.h>
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
    if (fsit.StartSearch(sSearch.GetData(), false, false).Succeeded())
    {
      do
      {
        ezStringBuilder sPlugin = fsit.GetStats().m_sFileName;
        sPlugin.RemoveFileExtension();

        if (sPlugin.FindSubString_NoCase("EnginePlugin") != nullptr || sPlugin.EndsWith_NoCase("Plugin"))
        {
          s_EnginePlugins.m_Plugins[sPlugin].m_bAvailable = true;
        }
      } while (fsit.Next().Succeeded());
    }
  }
#endif

  for (const auto& plugin : m_EnginePluginConfig.m_Plugins)
  {
    s_EnginePlugins.m_Plugins[plugin.m_sAppDirRelativePath].m_bActive = true;
    s_EnginePlugins.m_Plugins[plugin.m_sAppDirRelativePath].m_bToBeLoaded = plugin.m_sDependecyOf.Contains("<manual>");
    s_EnginePlugins.m_Plugins[plugin.m_sAppDirRelativePath].m_bLoadCopy = plugin.m_bLoadCopy;
  }
}

void ezQtEditorApp::StoreEnginePluginsToBeLoaded()
{
  bool bChange = false;

  for (auto it = s_EnginePlugins.m_Plugins.GetIterator(); it.IsValid(); ++it)
  {
    ezApplicationPluginConfig::PluginConfig cfg;
    cfg.m_sAppDirRelativePath = it.Key();

    if (it.Value().m_bToBeLoaded)
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
    m_EnginePluginConfig.Save();
  }
}


void ezQtEditorApp::ReadEnginePluginConfig()
{
  EZ_PROFILE("ReadEnginePluginConfig");
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
  m_EnginePluginConfig.Save();

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

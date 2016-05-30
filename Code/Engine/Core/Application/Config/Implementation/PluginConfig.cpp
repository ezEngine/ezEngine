#include <Core/PCH.h>
#include <Core/Application/Config/PluginConfig.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezApplicationPluginConfig, ezApplicationConfig, 1, ezRTTIDefaultAllocator<ezApplicationPluginConfig>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Plugins", m_Plugins),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezApplicationPluginConfig_PluginConfig, ezNoBase, 1, ezRTTIDefaultAllocator<ezApplicationPluginConfig_PluginConfig>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RelativePath", m_sRelativePath),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE


bool ezApplicationPluginConfig::PluginConfig::operator<(const PluginConfig& rhs) const
{
  return m_sRelativePath < rhs.m_sRelativePath;
}

bool ezApplicationPluginConfig::AddPlugin(const PluginConfig& cfg0)
{
  PluginConfig cfg = cfg0;

  if (cfg.m_sDependecyOf.IsEmpty())
  {
    cfg.m_sDependecyOf.Insert("<manual>");
  }

  for (ezUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    if (m_Plugins[i].m_sRelativePath == cfg.m_sRelativePath)
    {
      bool merged = false;

      for (auto it = cfg.m_sDependecyOf.GetIterator(); it.IsValid(); ++it)
      {
        if (!m_Plugins[i].m_sDependecyOf.Contains(*it))
        {
          m_Plugins[i].m_sDependecyOf.Insert(*it);
          merged = true;
        }
      }

      return merged;
    }
  }

  m_Plugins.PushBack(cfg);
  return true;
}


bool ezApplicationPluginConfig::RemovePlugin(const PluginConfig& cfg0)
{
  PluginConfig cfg = cfg0;

  if (cfg.m_sDependecyOf.IsEmpty())
    cfg.m_sDependecyOf.Insert("<manual>");

  bool modified = false;

  for (ezUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    if (m_Plugins[i].m_sRelativePath == cfg.m_sRelativePath)
    {
      for (auto it = cfg.m_sDependecyOf.GetIterator(); it.IsValid(); ++it)
      {
        if (m_Plugins[i].m_sDependecyOf.Remove(*it))
          modified = true;
      }

      if (m_Plugins[i].m_sDependecyOf.IsEmpty())
      {
        m_Plugins.RemoveAtSwap(i);
        return true;
      }

      return modified;
    }
  }

  return false;
}


ezApplicationPluginConfig::ezApplicationPluginConfig()
{
  m_bManualOnly = true;
}

ezResult ezApplicationPluginConfig::Save()
{
  m_Plugins.Sort();

  ezStringBuilder sPath;
  sPath = ":";
  sPath.AppendPath(GetProjectDirectory());
  sPath.AppendPath("Plugins.ezManifest");

  ezFileWriter file;
  if (file.Open(sPath).Failed())
    return EZ_FAILURE;

  ezStandardJSONWriter json;
  json.SetOutputStream(&file);

  json.BeginObject();

  json.BeginArray("Plugins");

  for (ezUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    json.BeginObject();

    json.AddVariableString("Path", m_Plugins[i].m_sRelativePath);

    if (!m_Plugins[i].m_sDependecyOf.IsEmpty())
    {
      json.BeginArray("DependencyOf");

      for (auto it = m_Plugins[i].m_sDependecyOf.GetIterator(); it.IsValid(); ++it)
      {
        json.WriteString(*it);
      }

      json.EndArray();
    }

    json.EndObject();
  }

  json.EndArray();

  json.EndObject();

  return EZ_SUCCESS;
}

void ezApplicationPluginConfig::Load()
{
  EZ_LOG_BLOCK("ezApplicationPluginConfig::Load()");

  m_Plugins.Clear();

  ezStringBuilder sPath;
  sPath = GetProjectDirectory();
  sPath.AppendPath("Plugins.ezManifest");

  ezFileReader file;
  if (file.Open(sPath).Failed())
  {
    ezLog::Warning("Could not open plugins config file '%s'", sPath.GetData());
    return;
  }

  ezJSONReader json;
  json.SetLogInterface(ezGlobalLog::GetOrCreateInstance());
  if (json.Parse(file).Failed())
  {
    ezLog::Error("Failed to parse plugins config file '%s'", sPath.GetData());
    return;
  }

  const auto& tree = json.GetTopLevelObject();

  ezVariant* dirs;
  if (!tree.TryGetValue("Plugins", dirs) || !dirs->IsA<ezVariantArray>())
  {
    ezLog::Error("Top level node is not an array");
    return;
  }

  for (auto& a : dirs->Get<ezVariantArray>())
  {
    if (!a.IsA<ezVariantDictionary>())
      continue;

    auto& datadir = a.Get<ezVariantDictionary>();

    ezVariant* pVar;

    PluginConfig cfg;

    if (datadir.TryGetValue("Path", pVar) && pVar->IsA<ezString>())
      cfg.m_sRelativePath = pVar->Get<ezString>();

    if (datadir.TryGetValue("DependencyOf", pVar) && pVar->IsA<ezVariantArray>())
    {
      const ezVariantArray& dep = pVar->Get<ezVariantArray>();

      for (ezUInt32 i = 0; i < dep.GetCount(); ++i)
      {
        if (dep[i].IsA<ezString>())
        {
          cfg.m_sDependecyOf.Insert(dep[i].Get<ezString>());
        }
      }
    }

    // this prevents duplicates
    AddPlugin(cfg);
  }
}

void ezApplicationPluginConfig::Apply()
{
  EZ_LOG_BLOCK("ezApplicationPluginConfig::Apply");

  for (const auto& var : m_Plugins)
  {
    if (m_bManualOnly)
    {
      if (!var.m_sDependecyOf.Contains("<manual>"))
        continue;
    }


    ezPlugin::LoadPlugin(var.m_sRelativePath);
  }

}



EZ_STATICLINK_FILE(Core, Core_Application_Config_Implementation_PluginConfig);


#include <Core/PCH.h>
#include <Core/Application/Config/PluginConfig.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Configuration/Plugin.h>

ezResult ezApplicationPluginConfig::Save()
{
  ezStringBuilder sPath;
  sPath = GetProjectDirectory();
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

    json.EndObject();
  }

  json.EndArray();

  json.EndObject();

  return EZ_SUCCESS;
}

void ezApplicationPluginConfig::Load()
{
  EZ_LOG_BLOCK("ezApplicationFileSystemConfig::Load()");

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
  json.SetLogInterface(ezGlobalLog::GetInstance());
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

    m_Plugins.PushBack(cfg);
  }
}

void ezApplicationPluginConfig::Apply()
{
  EZ_LOG_BLOCK("ezApplicationPluginConfig::Apply");

  for (const auto& var : m_Plugins)
  {
    ezPlugin::LoadPlugin(var.m_sRelativePath);
  }

}

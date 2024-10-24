#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezApplicationPluginConfig, ezNoBase, 1, ezRTTIDefaultAllocator<ezApplicationPluginConfig>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Plugins", m_Plugins),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezApplicationPluginConfig_PluginConfig, ezNoBase, 1, ezRTTIDefaultAllocator<ezApplicationPluginConfig_PluginConfig>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RelativePath", m_sAppDirRelativePath),
    EZ_MEMBER_PROPERTY("LoadCopy", m_bLoadCopy),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

bool ezApplicationPluginConfig::PluginConfig::operator<(const PluginConfig& rhs) const
{
  return m_sAppDirRelativePath < rhs.m_sAppDirRelativePath;
}

bool ezApplicationPluginConfig::AddPlugin(const PluginConfig& cfg0)
{
  PluginConfig cfg = cfg0;

  for (ezUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    if (m_Plugins[i].m_sAppDirRelativePath == cfg.m_sAppDirRelativePath)
    {
      return false;
    }
  }

  m_Plugins.PushBack(cfg);
  return true;
}

bool ezApplicationPluginConfig::RemovePlugin(const PluginConfig& cfg0)
{
  PluginConfig cfg = cfg0;

  for (ezUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    if (m_Plugins[i].m_sAppDirRelativePath == cfg.m_sAppDirRelativePath)
    {
      m_Plugins.RemoveAtAndSwap(i);
      return true;
    }
  }

  return false;
}

ezApplicationPluginConfig::ezApplicationPluginConfig() = default;

ezResult ezApplicationPluginConfig::Save(ezStringView sPath) const
{
  m_Plugins.Sort();

  ezDeferredFileWriter file;
  file.SetOutput(sPath, true);

  ezOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(ezOpenDdlWriter::TypeStringMode::Compliant);

  for (ezUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    writer.BeginObject("Plugin");

    ezOpenDdlUtils::StoreString(writer, m_Plugins[i].m_sAppDirRelativePath, "Path");
    ezOpenDdlUtils::StoreBool(writer, m_Plugins[i].m_bLoadCopy, "LoadCopy");

    writer.EndObject();
  }

  return file.Close();
}

void ezApplicationPluginConfig::Load(ezStringView sPath)
{
  EZ_LOG_BLOCK("ezApplicationPluginConfig::Load()");

  m_Plugins.Clear();

  ezFileReader file;
  if (file.Open(sPath).Failed())
  {
    ezLog::Warning("Could not open plugins config file '{0}'", sPath);
    return;
  }

  ezOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, ezLog::GetThreadLocalLogSystem()).Failed())
  {
    ezLog::Error("Failed to parse plugins config file '{0}'", sPath);
    return;
  }

  const ezOpenDdlReaderElement* pTree = reader.GetRootElement();

  for (const ezOpenDdlReaderElement* pPlugin = pTree->GetFirstChild(); pPlugin != nullptr; pPlugin = pPlugin->GetSibling())
  {
    if (!pPlugin->IsCustomType("Plugin"))
      continue;

    PluginConfig cfg;

    const ezOpenDdlReaderElement* pPath = pPlugin->FindChildOfType(ezOpenDdlPrimitiveType::String, "Path");
    const ezOpenDdlReaderElement* pCopy = pPlugin->FindChildOfType(ezOpenDdlPrimitiveType::Bool, "LoadCopy");

    if (pPath)
    {
      cfg.m_sAppDirRelativePath = pPath->GetPrimitivesString()[0];
    }

    if (pCopy)
    {
      cfg.m_bLoadCopy = pCopy->GetPrimitivesBool()[0];
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
    ezBitflags<ezPluginLoadFlags> flags;
    flags.AddOrRemove(ezPluginLoadFlags::LoadCopy, var.m_bLoadCopy);
    flags.AddOrRemove(ezPluginLoadFlags::CustomDependency, false);

    ezPlugin::LoadPlugin(var.m_sAppDirRelativePath, flags).IgnoreResult();
  }
}



EZ_STATICLINK_FILE(Foundation, Foundation_Application_Config_Implementation_PluginConfig);

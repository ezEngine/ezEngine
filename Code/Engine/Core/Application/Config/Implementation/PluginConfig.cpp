#include <PCH.h>

#include <Core/Application/Config/PluginConfig.h>
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

  if (cfg.m_sDependecyOf.IsEmpty())
  {
    cfg.m_sDependecyOf.Insert("<manual>");
  }

  for (ezUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    if (m_Plugins[i].m_sAppDirRelativePath == cfg.m_sAppDirRelativePath)
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
    if (m_Plugins[i].m_sAppDirRelativePath == cfg.m_sAppDirRelativePath)
    {
      for (auto it = cfg.m_sDependecyOf.GetIterator(); it.IsValid(); ++it)
      {
        if (m_Plugins[i].m_sDependecyOf.Remove(*it))
          modified = true;
      }

      if (m_Plugins[i].m_sDependecyOf.IsEmpty())
      {
        m_Plugins.RemoveAtAndSwap(i);
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
  sPath = ":project/Plugins.ddl";

  ezFileWriter file;
  if (file.Open(sPath).Failed())
    return EZ_FAILURE;

  ezOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(ezOpenDdlWriter::TypeStringMode::Compliant);

  for (ezUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    writer.BeginObject("Plugin");

    ezOpenDdlUtils::StoreString(writer, m_Plugins[i].m_sAppDirRelativePath, "Path");
    ezOpenDdlUtils::StoreBool(writer, m_Plugins[i].m_bLoadCopy, "LoadCopy");

    if (!m_Plugins[i].m_sDependecyOf.IsEmpty())
    {
      writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::String, "DependencyOf");

      for (auto it = m_Plugins[i].m_sDependecyOf.GetIterator(); it.IsValid(); ++it)
      {
        writer.WriteString(*it);
      }

      writer.EndPrimitiveList();
    }

    writer.EndObject();
  }

  return EZ_SUCCESS;
}

void ezApplicationPluginConfig::Load()
{
  EZ_LOG_BLOCK("ezApplicationPluginConfig::Load()");

  m_Plugins.Clear();

  ezStringBuilder sPath;
  sPath = ":project/Plugins.ddl";

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
    const ezOpenDdlReaderElement* pDependencyOf = pPlugin->FindChildOfType(ezOpenDdlPrimitiveType::String, "DependencyOf");
    const ezOpenDdlReaderElement* pCopy = pPlugin->FindChildOfType(ezOpenDdlPrimitiveType::Bool, "LoadCopy");

    if (pPath)
      cfg.m_sAppDirRelativePath = pPath->GetPrimitivesString()[0];

    if (pDependencyOf)
    {
      for (ezUInt32 i = 0; i < pDependencyOf->GetNumPrimitives(); ++i)
      {
        cfg.m_sDependecyOf.Insert(pDependencyOf->GetPrimitivesString()[i]);
      }
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
    if (m_bManualOnly)
    {
      if (!var.m_sDependecyOf.Contains("<manual>"))
        continue;
    }


    ezPlugin::LoadPlugin(var.m_sAppDirRelativePath, var.m_bLoadCopy);
  }
}



EZ_STATICLINK_FILE(Core, Core_Application_Config_Implementation_PluginConfig);

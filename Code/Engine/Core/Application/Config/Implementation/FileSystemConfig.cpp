#include <Core/PCH.h>
#include <Core/Application/Config/FileSystemConfig.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>

ezResult ezApplicationFileSystemConfig::Save()
{
  ezStringBuilder sPath;
  sPath = GetProjectDirectory();
  sPath.AppendPath("DataDirs.ezManifest");

  ezFileWriter file;
  if (file.Open(sPath).Failed())
    return EZ_FAILURE;

  ezStandardJSONWriter json;
  json.SetOutputStream(&file);

  json.BeginObject();

  json.BeginArray("DataDirectories");

  for (ezUInt32 i = 0; i < m_DataDirs.GetCount(); ++i)
  {
    json.BeginObject();

    json.AddVariableString("Path", m_DataDirs[i].m_sRelativePath);
    json.AddVariableBool("Writable", m_DataDirs[i].m_bWritable);

    json.EndObject();
  }

  json.EndArray();

  json.EndObject();

  return EZ_SUCCESS;
}

void ezApplicationFileSystemConfig::Load()
{
  EZ_LOG_BLOCK("ezApplicationFileSystemConfig::Load()");

  m_DataDirs.Clear();

  ezStringBuilder sPath;
  sPath = GetProjectDirectory();
  sPath.AppendPath("DataDirs.ezManifest");

  ezFileReader file;
  if (file.Open(sPath).Failed())
  {
    ezLog::Warning("Could not open file-system config file '%s'", sPath.GetData());
    return;
  }

  ezJSONReader json;
  json.SetLogInterface(ezGlobalLog::GetInstance());
  if (json.Parse(file).Failed())
  {
    ezLog::Error("Failed to parse file-system config file '%s'", sPath.GetData());
    return;
  }

  const auto& tree = json.GetTopLevelObject();
  
  ezVariant* dirs;
  if (!tree.TryGetValue("DataDirectories", dirs) || !dirs->IsA<ezVariantArray>())
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

    DataDirConfig cfg;
    cfg.m_bWritable = false;

    if (datadir.TryGetValue("Path", pVar) && pVar->IsA<ezString>())
      cfg.m_sRelativePath = pVar->Get<ezString>();
    if (datadir.TryGetValue("Writable", pVar) && pVar->IsA<bool>())
      cfg.m_bWritable = pVar->Get<bool>();

    m_DataDirs.PushBack(cfg);
  }
}

void ezApplicationFileSystemConfig::Apply()
{
  EZ_LOG_BLOCK("ezApplicationFileSystemConfig::Apply");

  for (const auto& var : m_DataDirs)
  {
    ezFileSystem::AddDataDirectory(var.m_sRelativePath, var.m_bWritable ? ezFileSystem::DataDirUsage::AllowWrites : ezFileSystem::DataDirUsage::ReadOnly, "AppFileSystemConfig");
  }
}



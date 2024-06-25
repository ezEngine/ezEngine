#include <Utilities/UtilitiesPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Utilities/Resources/ConfigFileResource.h>

static ezConfigFileResourceLoader s_ConfigFileResourceLoader;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Utilties, ConfigFileResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezResourceManager::SetResourceTypeLoader<ezConfigFileResource>(&s_ConfigFileResourceLoader);

    auto hFallback = ezResourceManager::LoadResource<ezConfigFileResource>("Empty.ezConfig");
    ezResourceManager::SetResourceTypeMissingFallback<ezConfigFileResource>(hFallback);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezResourceManager::SetResourceTypeMissingFallback<ezConfigFileResource>(ezConfigFileResourceHandle());
    ezResourceManager::SetResourceTypeLoader<ezConfigFileResource>(nullptr);
    ezConfigFileResource::CleanupDynamicPluginReferences();
  }

  EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConfigFileResource, 1, ezRTTIDefaultAllocator<ezConfigFileResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezConfigFileResource);

ezConfigFileResource::ezConfigFileResource()
  : ezResource(ezResource::DoUpdate::OnAnyThread, 0)
{
}

ezConfigFileResource::~ezConfigFileResource() = default;

ezInt32 ezConfigFileResource::GetInt(ezTempHashedString sName, ezInt32 iFallback) const
{
  auto it = m_IntData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return iFallback;
}

ezInt32 ezConfigFileResource::GetInt(ezTempHashedString sName) const
{
  auto it = m_IntData.Find(sName);
  if (it.IsValid())
    return it.Value();

  ezStringView name = "<unknown>"_ezsv;
  sName.LookupStringHash(name).IgnoreResult();
  ezLog::Error("{}: 'int' config variable '{}' doesn't exist.", this->GetResourceIdOrDescription(), name);
  return 0;
}

float ezConfigFileResource::GetFloat(ezTempHashedString sName, float fFallback) const
{
  auto it = m_FloatData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return fFallback;
}

float ezConfigFileResource::GetFloat(ezTempHashedString sName) const
{
  auto it = m_FloatData.Find(sName);
  if (it.IsValid())
    return it.Value();

  ezStringView name = "<unknown>"_ezsv;
  sName.LookupStringHash(name).IgnoreResult();
  ezLog::Error("{}: 'float' config variable '{}' doesn't exist.", this->GetResourceIdOrDescription(), name);
  return 0;
}

bool ezConfigFileResource::GetBool(ezTempHashedString sName, bool bFallback) const
{
  auto it = m_BoolData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return bFallback;
}

bool ezConfigFileResource::GetBool(ezTempHashedString sName) const
{
  auto it = m_BoolData.Find(sName);
  if (it.IsValid())
    return it.Value();

  ezStringView name = "<unknown>"_ezsv;
  sName.LookupStringHash(name).IgnoreResult();
  ezLog::Error("{}: 'float' config variable '{}' doesn't exist.", this->GetResourceIdOrDescription(), name);
  return false;
}

ezStringView ezConfigFileResource::GetString(ezTempHashedString sName, ezStringView sFallback) const
{
  auto it = m_StringData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return sFallback;
}

ezStringView ezConfigFileResource::GetString(ezTempHashedString sName) const
{
  auto it = m_StringData.Find(sName);
  if (it.IsValid())
    return it.Value();

  ezStringView name = "<unknown>"_ezsv;
  sName.LookupStringHash(name).IgnoreResult();
  ezLog::Error("{}: 'string' config variable '{}' doesn't exist.", this->GetResourceIdOrDescription(), name);
  return "";
}

ezResourceLoadDesc ezConfigFileResource::UnloadData(Unload WhatToUnload)
{
  EZ_IGNORE_UNUSED(WhatToUnload);

  m_IntData.Clear();
  m_FloatData.Clear();
  m_StringData.Clear();
  m_BoolData.Clear();

  ezResourceLoadDesc d;
  d.m_State = ezResourceState::Unloaded;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable = 0;
  return d;
}

ezResourceLoadDesc ezConfigFileResource::UpdateContent(ezStreamReader* Stream)
{
  ezResourceLoadDesc d;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable = 0;
  d.m_State = ezResourceState::Loaded;

  if (Stream == nullptr)
  {
    d.m_State = ezResourceState::LoadedResourceMissing;
    return d;
  }

  m_RequiredFiles.ReadDependencyFile(*Stream).IgnoreResult();
  Stream->ReadHashTable(m_IntData).IgnoreResult();
  Stream->ReadHashTable(m_FloatData).IgnoreResult();
  Stream->ReadHashTable(m_StringData).IgnoreResult();
  Stream->ReadHashTable(m_BoolData).IgnoreResult();

  return d;
}

void ezConfigFileResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = m_IntData.GetHeapMemoryUsage() + m_FloatData.GetHeapMemoryUsage() + m_StringData.GetHeapMemoryUsage() + m_BoolData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

//////////////////////////////////////////////////////////////////////////

ezResult ezConfigFileResourceLoader::LoadedData::PrePropFileLocator(ezStringView sCurAbsoluteFile, ezStringView sIncludeFile, ezPreprocessor::IncludeType incType, ezStringBuilder& out_sAbsoluteFilePath)
{
  ezResult res = ezPreprocessor::DefaultFileLocator(sCurAbsoluteFile, sIncludeFile, incType, out_sAbsoluteFilePath);

  m_RequiredFiles.AddFileDependency(out_sAbsoluteFilePath);

  return res;
}

ezResourceLoadData ezConfigFileResourceLoader::OpenDataStream(const ezResource* pResource)
{
  EZ_PROFILE_SCOPE("ReadResourceFile");
  EZ_LOG_BLOCK("Load Config Resource", pResource->GetResourceID());

  ezStringBuilder sConfig;

  ezMap<ezString, ezInt32> intData;
  ezMap<ezString, float> floatData;
  ezMap<ezString, ezString> stringData;
  ezMap<ezString, bool> boolData;

  LoadedData* pData = EZ_DEFAULT_NEW(LoadedData);
  pData->m_Reader.SetStorage(&pData->m_Storage);

  ezPreprocessor preprop;

  // used to gather all the transitive file dependencies
  preprop.SetFileLocatorFunction(ezMakeDelegate(&ezConfigFileResourceLoader::LoadedData::PrePropFileLocator, pData));

  if (ezStringUtils::IsEqual(pResource->GetResourceID(), "Empty.ezConfig"))
  {
    // do nothing
  }
  else if (preprop.Process(pResource->GetResourceID(), sConfig, false, true, false).Succeeded())
  {
    sConfig.ReplaceAll("\r", "");
    sConfig.ReplaceAll("\n", ";");

    ezHybridArray<ezStringView, 32> lines;
    sConfig.Split(false, lines, ";");

    ezStringBuilder key, value, line;

    for (ezStringView tmp : lines)
    {
      line = tmp;
      line.Trim(" \t");

      if (line.IsEmpty())
        continue;

      const char* szAssign = line.FindSubString("=");

      if (szAssign == nullptr)
      {
        ezLog::Error("Invalid line in config file: '{}'", tmp);
      }
      else
      {
        value = szAssign + 1;
        value.Trim(" ");

        line.SetSubString_FromTo(line.GetData(), szAssign);
        line.ReplaceAll("\t", " ");
        line.ReplaceAll("  ", " ");
        line.Trim(" ");

        const bool bOverride = line.TrimWordStart("override ");
        line.Trim(" ");

        if (line.StartsWith("int "))
        {
          key.SetSubString_FromTo(line.GetData() + 4, szAssign);
          key.Trim(" ");

          if (bOverride && !intData.Contains(key))
            ezLog::Error("Config 'int' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && intData.Contains(key))
            ezLog::Error("Config 'int' key '{}' is not marked override, but exist already. Use 'override int' instead.", key);

          ezInt32 val;
          if (ezConversionUtils::StringToInt(value, val).Succeeded())
          {
            intData[key] = val;
          }
          else
          {
            ezLog::Error("Failed to parse 'int' in config file: '{}'", tmp);
          }
        }
        else if (line.StartsWith("float "))
        {
          key.SetSubString_FromTo(line.GetData() + 6, szAssign);
          key.Trim(" ");

          if (bOverride && !floatData.Contains(key))
            ezLog::Error("Config 'float' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && floatData.Contains(key))
            ezLog::Error("Config 'float' key '{}' is not marked override, but exist already. Use 'override float' instead.", key);

          double val;
          if (ezConversionUtils::StringToFloat(value, val).Succeeded())
          {
            floatData[key] = (float)val;
          }
          else
          {
            ezLog::Error("Failed to parse 'float' in config file: '{}'", tmp);
          }
        }
        else if (line.StartsWith("bool "))
        {
          key.SetSubString_FromTo(line.GetData() + 5, szAssign);
          key.Trim(" ");

          if (bOverride && !boolData.Contains(key))
            ezLog::Error("Config 'bool' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && boolData.Contains(key))
            ezLog::Error("Config 'bool' key '{}' is not marked override, but exist already. Use 'override bool' instead.", key);

          bool val;
          if (ezConversionUtils::StringToBool(value, val).Succeeded())
          {
            boolData[key] = val;
          }
          else
          {
            ezLog::Error("Failed to parse 'bool' in config file: '{}'", tmp);
          }
        }
        else if (line.StartsWith("string "))
        {
          key.SetSubString_FromTo(line.GetData() + 7, szAssign);
          key.Trim(" ");

          if (bOverride && !stringData.Contains(key))
            ezLog::Error("Config 'string' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && stringData.Contains(key))
            ezLog::Error("Config 'string' key '{}' is not marked override, but exist already. Use 'override string' instead.", key);

          if (!value.StartsWith("\"") || !value.EndsWith("\""))
          {
            ezLog::Error("Failed to parse 'string' in config file: '{}'", tmp);
          }
          else
          {
            value.Shrink(1, 1);
            stringData[key] = value;
          }
        }
        else
        {
          ezLog::Error("Invalid line in config file: '{}'", tmp);
        }
      }
    }
  }
  else
  {
    // empty stream
    return {};
  }

  ezResourceLoadData res;
  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  ezFileStats stat;
  if (ezFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
  {
    res.m_sResourceDescription = stat.m_sName;
    res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
  }
#endif

  ezMemoryStreamWriter writer(&pData->m_Storage);

  pData->m_RequiredFiles.StoreCurrentTimeStamp();
  pData->m_RequiredFiles.WriteDependencyFile(writer).IgnoreResult();
  writer.WriteMap(intData).IgnoreResult();
  writer.WriteMap(floatData).IgnoreResult();
  writer.WriteMap(stringData).IgnoreResult();
  writer.WriteMap(boolData).IgnoreResult();

  return res;
}

void ezConfigFileResourceLoader::CloseDataStream(const ezResource* pResource, const ezResourceLoadData& loaderData)
{
  EZ_IGNORE_UNUSED(pResource);

  LoadedData* pData = static_cast<LoadedData*>(loaderData.m_pCustomLoaderData);

  EZ_DEFAULT_DELETE(pData);
}

bool ezConfigFileResourceLoader::IsResourceOutdated(const ezResource* pResource) const
{
  return static_cast<const ezConfigFileResource*>(pResource)->m_RequiredFiles.HasAnyFileChanged();
}


EZ_STATICLINK_FILE(Utilities, Utilities_Resources_ConfigFileResource);

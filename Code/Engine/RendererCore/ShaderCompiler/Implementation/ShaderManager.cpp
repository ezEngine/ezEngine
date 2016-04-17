#include <RendererCore/PCH.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>

bool ezShaderManager::s_bEnableRuntimeCompilation = false;
ezString ezShaderManager::s_sPlatform;
ezString ezShaderManager::s_sPermVarSubDir;
ezString ezShaderManager::s_ShaderCacheDirectory;

namespace
{
  struct PermutationVarConfig
  {
    ezHashedString m_sName;
    ezVariant m_DefaultValue;
    ezDynamicArray<ezHashedString> m_EnumValues;
  };

  static ezHashTable<ezHashedString, PermutationVarConfig> s_PermutationVarConfigs;
  static ezDynamicArray<ezPermutationVar> s_FilteredPermutationVariables;
  static ezMap<ezUInt32, ezDynamicArray<ezPermutationVar> > s_PermutationVarsMap;

  const PermutationVarConfig* FindConfig(const ezHashedString& sName)
  {
    PermutationVarConfig* pConfig = nullptr;
    if (!s_PermutationVarConfigs.TryGetValue(sName, pConfig))
    {
      ezShaderManager::ReloadPermutationVarConfig(sName);
      s_PermutationVarConfigs.TryGetValue(sName, pConfig);
    }

    return pConfig;
  }

  bool IsValueAllowed(const PermutationVarConfig& config, const ezHashedString& sValue)
  {
    const char* szValue = sValue.GetData();
    if (config.m_DefaultValue.IsA<bool>())
    {
      return ezStringUtils::IsEqual(szValue, "TRUE") || ezStringUtils::IsEqual(szValue, "FALSE") ||
        ezStringUtils::IsEqual(szValue, "1") || ezStringUtils::IsEqual(szValue, "0");
    }
    else
    {
      for (auto& enumValue : config.m_EnumValues)
      {
        if (enumValue == sValue)
          return true;
      }

      return false;
    }
  }
}

void ezShaderManager::Configure(const char* szActivePlatform, bool bEnableRuntimeCompilation, const char* szShaderCacheDirectory, const char* szPermVarSubDirectory)
{
  s_ShaderCacheDirectory = szShaderCacheDirectory;
  s_sPermVarSubDir = szPermVarSubDirectory;

  ezStringBuilder s = szActivePlatform;
  s.ToUpper();

  s_bEnableRuntimeCompilation = bEnableRuntimeCompilation;
  s_sPlatform = s;
}

void ezShaderManager::ReloadPermutationVarConfig(const ezHashedString& sName)
{
  ezStringBuilder sPath;
  sPath.Format("%s/%s.ezPermVar", s_sPermVarSubDir.GetData(), sName.GetData());

  // clear earlier data
  s_PermutationVarConfigs.Remove(sName);

  ezStringBuilder sTemp = s_sPlatform;
  sTemp.Append(" 1");

  ezPreprocessor pp;
  pp.SetLogInterface(ezGlobalLog::GetOrCreateInstance());
  pp.SetPassThroughLine(false);
  pp.SetPassThroughPragma(false);
  pp.AddCustomDefine(sTemp.GetData());

  if (pp.Process(sPath, sTemp, false).Failed())
  {
    ezLog::Error("Could not read shader permutation variable '%s' from file '%s'", sName.GetData(), sPath.GetData());
  }

  ezVariant defaultValue;
  ezHybridArray<ezHashedString, 16> enumValues;

  ezShaderHelper::ParsePermutationVarConfig(sTemp, defaultValue, enumValues);
  if (defaultValue.IsValid())
  {
    auto& config = s_PermutationVarConfigs[sName];
    config.m_sName = sName;
    config.m_DefaultValue = defaultValue;
    config.m_EnumValues = enumValues;
  }
}


bool ezShaderManager::IsPermutationValueAllowed(const ezHashedString& sName, const ezHashedString& sValue)
{
  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig == nullptr)
  {
    ezLog::Error("Permutation variable '%s' does not exist", sName.GetData());
    return false;
  }

  if (!IsValueAllowed(*pConfig, sValue))
  {
    if (!s_bEnableRuntimeCompilation)
    {
      return false;
    }
    
    ezLog::Debug("Invalid Shader Permutation: '%s' cannot be set to value '%s' -> reloading config for variable", sName.GetData(), sValue.GetData());
    ReloadPermutationVarConfig(sName);

    if (!IsValueAllowed(*pConfig, sValue))
    {
      ezLog::Error("Invalid Shader Permutation: '%s' cannot be set to value '%s'", sName.GetData(), sValue.GetData());
      return false;
    }
  }

  return true;
}


ezArrayPtr<const ezHashedString> ezShaderManager::GetPermutationEnumValues(const ezHashedString& sName)
{
  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig != nullptr)
  {
    return pConfig->m_EnumValues;
  }

  return ezArrayPtr<ezHashedString>();
}

void ezShaderManager::PreloadPermutations(ezShaderResourceHandle hShader, const ezHashTable<ezHashedString, ezHashedString>& permVars, ezTime tShouldBeAvailableIn)
{
  ezResourceLock<ezShaderResource> pShader(hShader, ezResourceAcquireMode::NoFallback);

  if (!pShader->IsShaderValid())
    return;

  ezUInt32 uiPermutationHash = FilterPermutationVars(pShader->GetUsedPermutationVars(), permVars);

  EZ_ASSERT_NOT_IMPLEMENTED;
#if 0
  generator.RemoveUnusedPermutations(pShader->GetUsedPermutationVars());

  ezHybridArray<ezPermutationVar, 16> usedPermVars;

  const ezUInt32 uiPermutationCount = generator.GetPermutationCount();
  for (ezUInt32 uiPermutation = 0; uiPermutation < uiPermutationCount; ++uiPermutation)
  {
    generator.GetPermutation(uiPermutation, usedPermVars);

    PreloadSingleShaderPermutation(hShader, usedPermVars, tShouldBeAvailableIn);
  }
#endif
}

ezShaderPermutationResourceHandle ezShaderManager::PreloadSinglePermutation(ezShaderResourceHandle hShader, 
  const ezHashTable<ezHashedString, ezHashedString>& permVars, ezTime tShouldBeAvailableIn)
{
  ezResourceLock<ezShaderResource> pShader(hShader, ezResourceAcquireMode::NoFallback);

  if (!pShader->IsShaderValid())
    return ezShaderPermutationResourceHandle();

  ezUInt32 uiPermutationHash = FilterPermutationVars(pShader->GetUsedPermutationVars(), permVars);

  return PreloadSinglePermutationInternal(pShader->GetResourceID(), uiPermutationHash, tShouldBeAvailableIn);
}


ezArrayPtr<ezPermutationVar> ezShaderManager::GetPermutationVars(ezUInt32 uiHash)
{
  auto it = s_PermutationVarsMap.Find(uiHash);
  if (it.IsValid())
  {
    return it.Value();
  }

  return ezArrayPtr<ezPermutationVar>();
}

ezUInt32 ezShaderManager::FilterPermutationVars(const ezArrayPtr<const ezHashedString>& usedVars, const ezHashTable<ezHashedString, ezHashedString>& permVars)
{
  s_FilteredPermutationVariables.Clear();

  for (auto& sName : usedVars)
  {
    auto& var = s_FilteredPermutationVariables.ExpandAndGetRef();
    var.m_sName = sName;

    if (!permVars.TryGetValue(sName, var.m_sValue))
    {
      const PermutationVarConfig* pConfig = FindConfig(sName);
      if (pConfig == nullptr)
        continue;

      const ezVariant& defaultValue = pConfig->m_DefaultValue;
      if (defaultValue.IsA<bool>())
      {
        var.m_sValue.Assign(defaultValue.Get<bool>() ? "TRUE" : "FALSE");
      }
      else
      {
        ezUInt32 uiDefaultValue = defaultValue.Get<ezUInt32>();
        var.m_sValue = pConfig->m_EnumValues[uiDefaultValue];
      }
    }
  }

  ezUInt32 uiPermutationHash = ezShaderHelper::CalculateHash(s_FilteredPermutationVariables);

  s_PermutationVarsMap[uiPermutationHash] = s_FilteredPermutationVariables;

  return uiPermutationHash;
}



ezShaderPermutationResourceHandle ezShaderManager::PreloadSinglePermutationInternal(const char* szResourceId, ezUInt32 uiPermutationHash, ezTime tShouldBeAvailableIn)
{
  ezStringBuilder sShaderFile = GetCacheDirectory();
  sShaderFile.AppendPath(GetActivePlatform().GetData());
  sShaderFile.AppendPath(szResourceId);
  sShaderFile.ChangeFileExtension("");
  if (sShaderFile.EndsWith("."))
    sShaderFile.Shrink(0, 1);
  sShaderFile.AppendFormat("%08X.ezPermutation", uiPermutationHash);

  ezShaderPermutationResourceHandle hShaderPermutation = ezResourceManager::LoadResource<ezShaderPermutationResource>(sShaderFile.GetData());
  
  ezResourceManager::PreloadResource(hShaderPermutation, tShouldBeAvailableIn);

  return hShaderPermutation;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderManager);


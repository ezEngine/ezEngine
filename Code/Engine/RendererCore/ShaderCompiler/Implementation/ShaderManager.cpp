#include <RendererCorePCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

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
    ezDynamicArray<ezHashedString, ezStaticAllocatorWrapper> m_EnumValues;
  };

  static ezDeque<PermutationVarConfig, ezStaticAllocatorWrapper> s_PermutationVarConfigsStorage;
  static ezHashTable<ezHashedString, PermutationVarConfig*> s_PermutationVarConfigs;
  static ezMutex s_PermutationVarConfigsMutex;

  const PermutationVarConfig* FindConfig(const char* szName, const ezTempHashedString& sHashedName)
  {
    EZ_LOCK(s_PermutationVarConfigsMutex);

    PermutationVarConfig* pConfig = nullptr;
    if (!s_PermutationVarConfigs.TryGetValue(sHashedName, pConfig))
    {
      ezShaderManager::ReloadPermutationVarConfig(szName, sHashedName);
      s_PermutationVarConfigs.TryGetValue(sHashedName, pConfig);
    }

    return pConfig;
  }

  const PermutationVarConfig* FindConfig(const ezHashedString& sName)
  {
    EZ_LOCK(s_PermutationVarConfigsMutex);

    PermutationVarConfig* pConfig = nullptr;
    if (!s_PermutationVarConfigs.TryGetValue(sName, pConfig))
    {
      ezShaderManager::ReloadPermutationVarConfig(sName.GetData(), sName);
      s_PermutationVarConfigs.TryGetValue(sName, pConfig);
    }

    return pConfig;
  }

  static ezHashedString s_sTrue = ezMakeHashedString("TRUE");
  static ezHashedString s_sFalse = ezMakeHashedString("FALSE");

  bool IsValueAllowed(const PermutationVarConfig& config, const ezTempHashedString& sValue, ezHashedString& out_sValue)
  {
    if (config.m_DefaultValue.IsA<bool>())
    {
      if (sValue == ezTempHashedString("TRUE"))
      {
        out_sValue = s_sTrue;
        return true;
      }

      if (sValue == ezTempHashedString("FALSE"))
      {
        out_sValue = s_sFalse;
        return true;
      }
    }
    else
    {
      for (auto& enumValue : config.m_EnumValues)
      {
        if (enumValue == sValue)
        {
          out_sValue = enumValue;
          return true;
        }
      }
    }

    return false;
  }

  bool IsValueAllowed(const PermutationVarConfig& config, const ezTempHashedString& sValue)
  {
    if (config.m_DefaultValue.IsA<bool>())
    {
      return sValue == ezTempHashedString("TRUE") || sValue == ezTempHashedString("FALSE");
    }
    else
    {
      for (auto& enumValue : config.m_EnumValues)
      {
        if (enumValue == sValue)
          return true;
      }
    }

    return false;
  }

  static ezHashTable<ezUInt64, ezString> s_PermutationPaths;
} // namespace

//////////////////////////////////////////////////////////////////////////

void ezShaderManager::Configure(
  const char* szActivePlatform, bool bEnableRuntimeCompilation, const char* szShaderCacheDirectory, const char* szPermVarSubDirectory)
{
  s_ShaderCacheDirectory = szShaderCacheDirectory;
  s_sPermVarSubDir = szPermVarSubDirectory;

  ezStringBuilder s = szActivePlatform;
  s.ToUpper();

  s_bEnableRuntimeCompilation = bEnableRuntimeCompilation;
  s_sPlatform = s;
}

void ezShaderManager::ReloadPermutationVarConfig(const char* szName, const ezTempHashedString& sHashedName)
{
  // clear earlier data
  {
    EZ_LOCK(s_PermutationVarConfigsMutex);

    s_PermutationVarConfigs.Remove(sHashedName);
  }

  ezStringBuilder sPath;
  sPath.Format("{0}/{1}.ezPermVar", s_sPermVarSubDir, szName);

  ezStringBuilder sTemp = s_sPlatform;
  sTemp.Append(" 1");

  ezPreprocessor pp;
  pp.SetLogInterface(ezLog::GetThreadLocalLogSystem());
  pp.SetPassThroughLine(false);
  pp.SetPassThroughPragma(false);
  pp.AddCustomDefine(sTemp.GetData());

  if (pp.Process(sPath, sTemp, false).Failed())
  {
    ezLog::Error("Could not read shader permutation variable '{0}' from file '{1}'", szName, sPath);
  }

  ezVariant defaultValue;
  ezShaderParser::EnumDefinition enumDef;

  ezShaderParser::ParsePermutationVarConfig(sTemp, defaultValue, enumDef);
  if (defaultValue.IsValid())
  {
    EZ_LOCK(s_PermutationVarConfigsMutex);

    auto pConfig = &s_PermutationVarConfigsStorage.ExpandAndGetRef();
    pConfig->m_sName.Assign(szName);
    pConfig->m_DefaultValue = defaultValue;
    pConfig->m_EnumValues = enumDef.m_Values;
    
    s_PermutationVarConfigs.Insert(pConfig->m_sName, pConfig);
  }
}

bool ezShaderManager::IsPermutationValueAllowed(const char* szName, const ezTempHashedString& sHashedName, const ezTempHashedString& sValue,
  ezHashedString& out_sName, ezHashedString& out_sValue)
{
  const PermutationVarConfig* pConfig = FindConfig(szName, sHashedName);
  if (pConfig == nullptr)
  {
    ezLog::Error("Permutation variable '{0}' does not exist", szName);
    return false;
  }

  out_sName = pConfig->m_sName;

  if (!IsValueAllowed(*pConfig, sValue, out_sValue))
  {
    if (!s_bEnableRuntimeCompilation)
    {
      return false;
    }

    ezLog::Debug(
      "Invalid Shader Permutation: '{0}' cannot be set to value '{1}' -> reloading config for variable", szName, sValue.GetHash());
    ReloadPermutationVarConfig(szName, sHashedName);

    if (!IsValueAllowed(*pConfig, sValue, out_sValue))
    {
      ezLog::Error("Invalid Shader Permutation: '{0}' cannot be set to value '{1}'", szName, sValue.GetHash());
      return false;
    }
  }

  return true;
}

bool ezShaderManager::IsPermutationValueAllowed(const ezHashedString& sName, const ezHashedString& sValue)
{
  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig == nullptr)
  {
    ezLog::Error("Permutation variable '{0}' does not exist", sName);
    return false;
  }

  if (!IsValueAllowed(*pConfig, sValue))
  {
    if (!s_bEnableRuntimeCompilation)
    {
      return false;
    }

    ezLog::Debug("Invalid Shader Permutation: '{0}' cannot be set to value '{1}' -> reloading config for variable", sName, sValue);
    ReloadPermutationVarConfig(sName, sName);

    if (!IsValueAllowed(*pConfig, sValue))
    {
      ezLog::Error("Invalid Shader Permutation: '{0}' cannot be set to value '{1}'", sName, sValue);
      return false;
    }
  }

  return true;
}

void ezShaderManager::GetPermutationValues(const ezHashedString& sName, ezHybridArray<ezHashedString, 4>& out_Values)
{
  out_Values.Clear();

  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig == nullptr)
    return;

  if (pConfig->m_DefaultValue.IsA<bool>())
  {
    out_Values.PushBack(s_sTrue);
    out_Values.PushBack(s_sFalse);
  }
  else
  {
    for (const auto& val : pConfig->m_EnumValues)
    {
      out_Values.PushBack(val);
    }
  }
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

void ezShaderManager::PreloadPermutations(
  ezShaderResourceHandle hShader, const ezHashTable<ezHashedString, ezHashedString>& permVars, ezTime tShouldBeAvailableIn)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
#if 0
  ezResourceLock<ezShaderResource> pShader(hShader, ezResourceAcquireMode::BlockTillLoaded);

  if (!pShader->IsShaderValid())
    return;

  /*ezUInt32 uiPermutationHash = */ FilterPermutationVars(pShader->GetUsedPermutationVars(), permVars);

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
  const ezHashTable<ezHashedString, ezHashedString>& permVars, bool bAllowFallback)
{
  ezResourceLock<ezShaderResource> pShader(
    hShader, bAllowFallback ? ezResourceAcquireMode::AllowLoadingFallback : ezResourceAcquireMode::BlockTillLoaded);

  if (!pShader->IsShaderValid())
    return ezShaderPermutationResourceHandle();

  ezHybridArray<ezPermutationVar, 64> filteredPermutationVariables(ezFrameAllocator::GetCurrentAllocator());
  ezUInt32 uiPermutationHash = FilterPermutationVars(pShader->GetUsedPermutationVars(), permVars, filteredPermutationVariables);

  return PreloadSinglePermutationInternal(pShader->GetResourceID(), pShader->GetResourceIDHash(), uiPermutationHash, filteredPermutationVariables);
}


ezUInt32 ezShaderManager::FilterPermutationVars(ezArrayPtr<const ezHashedString> usedVars, const ezHashTable<ezHashedString, ezHashedString>& permVars,
  ezDynamicArray<ezPermutationVar>& out_FilteredPermutationVariables)
{
  for (auto& sName : usedVars)
  {
    auto& var = out_FilteredPermutationVariables.ExpandAndGetRef();
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

  return ezShaderHelper::CalculateHash(out_FilteredPermutationVariables);
}



ezShaderPermutationResourceHandle ezShaderManager::PreloadSinglePermutationInternal(const char* szResourceId, ezUInt32 uiResourceIdHash,
  ezUInt32 uiPermutationHash, ezArrayPtr<ezPermutationVar> filteredPermutationVariables)
{
  const ezUInt64 uiPermutationKey = (ezUInt64)uiResourceIdHash << 32 | uiPermutationHash;

  ezString* pPermutationPath = &s_PermutationPaths[uiPermutationKey];
  if (pPermutationPath->IsEmpty())
  {
    ezStringBuilder sShaderFile = GetCacheDirectory();
    sShaderFile.AppendPath(GetActivePlatform().GetData());
    sShaderFile.AppendPath(szResourceId);
    sShaderFile.ChangeFileExtension("");
    if (sShaderFile.EndsWith("."))
      sShaderFile.Shrink(0, 1);
    sShaderFile.AppendFormat("_{0}.ezPermutation", ezArgU(uiPermutationHash, 8, true, 16, true));

    *pPermutationPath = sShaderFile;
  }

  ezShaderPermutationResourceHandle hShaderPermutation =
    ezResourceManager::LoadResource<ezShaderPermutationResource>(pPermutationPath->GetData());

  {
    ezResourceLock<ezShaderPermutationResource> pShaderPermutation(hShaderPermutation, ezResourceAcquireMode::PointerOnly);
    if (!pShaderPermutation->IsShaderValid())
    {
      pShaderPermutation->m_PermutationVars = filteredPermutationVariables;
    }
  }

  ezResourceManager::PreloadResource(hShaderPermutation);

  return hShaderPermutation;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderManager);

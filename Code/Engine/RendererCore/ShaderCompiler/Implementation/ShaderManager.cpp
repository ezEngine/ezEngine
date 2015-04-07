#include <RendererCore/PCH.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <RendererFoundation/Context/Context.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>

const ezPermutationGenerator* ezRenderContext::GetGeneratorForShaderPermutation(ezUInt32 uiPermutationHash)
{
  auto it = s_PermutationHashCache.Find(uiPermutationHash);

  if (it.IsValid())
    return &it.Value();

  return nullptr;
}

void ezRenderContext::LoadShaderPermutationVarConfig(const char* szVariable)
{
  ezStringBuilder sPath;
  sPath.Format("%s/%s.ezPermVar", s_sPermVarSubDir.GetData(), szVariable);

  // clear earlier data
  s_AllowedPermutations.RemoveVariable(szVariable);

  if (s_AllowedPermutations.ReadFromFile(sPath, s_sPlatform).Failed())
    ezLog::Error("Could not read shader permutation variable '%s' from file '%s'", szVariable, sPath.GetData());
}

void ezRenderContext::PreloadShaderPermutations(ezShaderResourceHandle hShader, const ezPermutationGenerator& MainGenerator, ezTime tShouldBeAvailableIn)
{
  ezResourceLock<ezShaderResource> pShader(hShader, ezResourceAcquireMode::NoFallback);

  ezPermutationGenerator Generator = MainGenerator;
  Generator.RemoveUnusedPermutations(pShader->GetUsedPermutationVars());

  ezHybridArray<ezPermutationGenerator::PermutationVar, 16> UsedPermVars;

  for (ezUInt32 p = 0; p < Generator.GetPermutationCount(); ++p)
  {
    Generator.GetPermutation(p, UsedPermVars);

    PreloadSingleShaderPermutation(hShader, UsedPermVars, tShouldBeAvailableIn);
  }
}

ezShaderPermutationResourceHandle ezRenderContext::PreloadSingleShaderPermutation(ezShaderResourceHandle hShader, const ezHybridArray<ezPermutationGenerator::PermutationVar, 16>& UsedPermVars, ezTime tShouldBeAvailableIn)
{
  ezResourceLock<ezShaderResource> pShader(hShader, ezResourceAcquireMode::NoFallback);

  if (!pShader->IsShaderValid())
    return ezShaderPermutationResourceHandle();

  const ezUInt32 uiPermutationHash = ezPermutationGenerator::GetHash(UsedPermVars);

  /// \todo Mutex

  bool bExisted = false;
  auto itPermCache = s_PermutationHashCache.FindOrAdd(uiPermutationHash, &bExisted);

  if (!bExisted)
  {
    // store this set of permutations in a generator
    for (ezUInt32 pv = 0; pv < UsedPermVars.GetCount(); ++pv)
      itPermCache.Value().AddPermutation(UsedPermVars[pv].m_sVariable.GetData(), UsedPermVars[pv].m_sValue.GetData());
  }

  ezStringBuilder sShaderFile = GetShaderCacheDirectory();
  sShaderFile.AppendPath(GetActiveShaderPlatform().GetData());
  sShaderFile.AppendPath(pShader->GetResourceID().GetData());
  sShaderFile.ChangeFileExtension("");
  if (sShaderFile.EndsWith("."))
    sShaderFile.Shrink(0, 1);
  sShaderFile.AppendFormat("%08X.ezPermutation", uiPermutationHash);

  ezShaderPermutationResourceHandle hShaderPermutation = ezResourceManager::LoadResource<ezShaderPermutationResource>(sShaderFile.GetData());

  ezResourceManager::PreloadResource(hShaderPermutation, tShouldBeAvailableIn);

  return hShaderPermutation;
}

void ezRenderContext::ConfigureShaderSystem(const char* szActivePlatform, bool bEnableRuntimeCompilation, const char* szShaderCacheDirectory, const char* szPermVarSubDirectory)
{
  s_ShaderCacheDirectory = szShaderCacheDirectory;
  s_sPermVarSubDir = szPermVarSubDirectory;

  ezStringBuilder s = szActivePlatform;
  s.ToUpper();

  s_bEnableRuntimeCompilation = bEnableRuntimeCompilation;
  s_sPlatform = s;
}

void ezRenderContext::SetShaderPermutationVariable(const char* szVariable, const char* szValue)
{
  ezStringBuilder sVar, sVal;
  sVar = szVariable;
  sVal = szValue;

  sVar.ToUpper();
  sVal.ToUpper();

  /// \todo Could we use hashed variable names here ?
  bool bExisted = false;
  auto itVar = m_ContextState.m_PermutationVariables.FindOrAdd(sVar, &bExisted);

  if (!bExisted)
  {
    LoadShaderPermutationVarConfig(sVar);
  }

  if (s_bEnableRuntimeCompilation && !s_AllowedPermutations.IsValueAllowed(sVar.GetData(), sVal.GetData()))
  {
    ezLog::Debug("Invalid Shader Permutation: '%s' cannot be set to value '%s' -> reloading config for variable", sVar.GetData(), sVal.GetData());
    LoadShaderPermutationVarConfig(sVar);

    if (!s_AllowedPermutations.IsValueAllowed(sVar.GetData(), sVal.GetData()))
    {
      ezLog::Error("Invalid Shader Permutation: '%s' cannot be set to value '%s'", sVar.GetData(), sVal.GetData());
      return;
    }
  }

  if (itVar.Value() != sVal)
    m_ContextState.m_bShaderStateChanged = true;

  itVar.Value() = sVal;
}

void ezRenderContext::SetActiveShader(ezShaderResourceHandle hShader, ezBitflags<ezShaderBindFlags> flags)
{
  m_ContextState.m_ShaderBindFlags = flags;

  if (flags.IsAnySet(ezShaderBindFlags::ForceRebind) || m_ContextState.m_hActiveShader != hShader)
    m_ContextState.m_bShaderStateChanged = true;

  m_ContextState.m_hActiveShader = hShader;
}

ezGALShaderHandle ezRenderContext::GetActiveGALShader()
{
  // make sure the internal state is up to date
  SetShaderContextState(false);

  if (!m_ContextState.m_bShaderStateValid)
    return ezGALShaderHandle(); // invalid handle

  return m_ContextState.m_hActiveGALShader;
}

void ezRenderContext::SetShaderContextState(bool bForce)
{
  ezShaderPermutationResource* pShaderPermutation = nullptr;

  if (bForce || m_ContextState.m_bShaderStateChanged)
  {
    m_ContextState.m_bShaderStateChanged = false;
    m_ContextState.m_bShaderStateValid = false;
    m_ContextState.m_bTextureBindingsChanged = true;
    m_ContextState.m_bConstantBufferBindingsChanged = true;

    if (!m_ContextState.m_hActiveShader.IsValid())
      return;

    ezResourceLock<ezShaderResource> pShader(m_ContextState.m_hActiveShader, ezResourceAcquireMode::AllowFallback);

    if (!pShader->IsShaderValid())
      return;

    m_ContextState.m_PermGenerator.Clear();
    for (auto itPerm = m_ContextState.m_PermutationVariables.GetIterator(); itPerm.IsValid(); ++itPerm)
      m_ContextState.m_PermGenerator.AddPermutation(itPerm.Key().GetData(), itPerm.Value().GetData());

    m_ContextState.m_PermGenerator.RemoveUnusedPermutations(pShader->GetUsedPermutationVars());

    EZ_ASSERT_DEV(m_ContextState.m_PermGenerator.GetPermutationCount() == 1, "Invalid shader setup");

    ezHybridArray<ezPermutationGenerator::PermutationVar, 16> UsedPermVars;
    m_ContextState.m_PermGenerator.GetPermutation(0, UsedPermVars);

    m_ContextState.m_hActiveShaderPermutation = PreloadSingleShaderPermutation(m_ContextState.m_hActiveShader, UsedPermVars, ezTime::Seconds(0.0));

    if (!m_ContextState.m_hActiveShaderPermutation.IsValid())
      return;

    pShaderPermutation = ezResourceManager::BeginAcquireResource(m_ContextState.m_hActiveShaderPermutation, ezResourceAcquireMode::AllowFallback);

    if (!pShaderPermutation->IsShaderValid())
    {
      ezResourceManager::EndAcquireResource(pShaderPermutation);
      return;
    }

    m_ContextState.m_hActiveGALShader = pShaderPermutation->GetGALShader();

    m_pGALContext->SetShader(m_ContextState.m_hActiveGALShader);

    // Set render state from shader (unless they are all deactivated)
    if (!m_ContextState.m_ShaderBindFlags.AreAllSet(ezShaderBindFlags::NoBlendState | ezShaderBindFlags::NoRasterizerState | ezShaderBindFlags::NoDepthStencilState))
    {
      if (!m_ContextState.m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoBlendState))
        m_pGALContext->SetBlendState(pShaderPermutation->GetBlendState());

      if (!m_ContextState.m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoRasterizerState))
        m_pGALContext->SetRasterizerState(pShaderPermutation->GetRasterizerState());

      if (!m_ContextState.m_ShaderBindFlags.IsSet(ezShaderBindFlags::NoDepthStencilState))
        m_pGALContext->SetDepthStencilState(pShaderPermutation->GetDepthStencilState());
    }

    m_ContextState.m_bShaderStateValid = true;
  }

  if ((bForce || m_ContextState.m_bTextureBindingsChanged) && m_ContextState.m_hActiveShaderPermutation.IsValid())
  {
    m_ContextState.m_bTextureBindingsChanged = false;

    if (pShaderPermutation == nullptr)
      pShaderPermutation = ezResourceManager::BeginAcquireResource(m_ContextState.m_hActiveShaderPermutation, ezResourceAcquireMode::AllowFallback);

    for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      auto pBin = pShaderPermutation->GetShaderStageBinary((ezGALShaderStage::Enum) stage);

      if (pBin == nullptr)
        continue;

      ApplyTextureBindings((ezGALShaderStage::Enum) stage, pBin);
    }
  }

  UploadGlobalConstants();

  if ((bForce || m_ContextState.m_bConstantBufferBindingsChanged) && m_ContextState.m_hActiveShaderPermutation.IsValid())
  {
    m_ContextState.m_bConstantBufferBindingsChanged = false;

    if (pShaderPermutation == nullptr)
      pShaderPermutation = ezResourceManager::BeginAcquireResource(m_ContextState.m_hActiveShaderPermutation, ezResourceAcquireMode::AllowFallback);

    for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      auto pBin = pShaderPermutation->GetShaderStageBinary((ezGALShaderStage::Enum) stage);

      if (pBin == nullptr)
        continue;

      ApplyConstantBufferBindings(pBin);
    }
  }

  if (pShaderPermutation != nullptr)
    ezResourceManager::EndAcquireResource(pShaderPermutation);
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderManager);


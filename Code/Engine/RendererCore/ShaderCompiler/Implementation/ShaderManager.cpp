#include <RendererCore/PCH.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <RendererFoundation/Context/Context.h>
#include <Core/ResourceManager/ResourceManager.h>

const ezPermutationGenerator* ezShaderManager::GetGeneratorForPermutation(ezUInt32 uiPermutationHash)
{
  auto it = s_PermutationHashCache.Find(uiPermutationHash);

  if (it.IsValid())
    return &it.Value();

  return nullptr;
}

void ezShaderManager::PreloadPermutations(ezShaderResourceHandle hShader, const ezPermutationGenerator& MainGenerator, ezTime tShouldBeAvailableIn)
{
  ezResourceLock<ezShaderResource> pShader(hShader, ezResourceAcquireMode::NoFallback);

  ezPermutationGenerator Generator = MainGenerator;
  Generator.RemoveUnusedPermutations(pShader->GetUsedPermutationVars());

  ezHybridArray<ezPermutationGenerator::PermutationVar, 16> UsedPermVars;

  for (ezUInt32 p = 0; p < Generator.GetPermutationCount(); ++p)
  {
    Generator.GetPermutation(p, UsedPermVars);

    PreloadSinglePermutation(hShader, UsedPermVars, tShouldBeAvailableIn);
  }
}

ezShaderPermutationResourceHandle ezShaderManager::PreloadSinglePermutation(ezShaderResourceHandle hShader, const ezHybridArray<ezPermutationGenerator::PermutationVar, 16>& UsedPermVars, ezTime tShouldBeAvailableIn)
{
  ezResourceLock<ezShaderResource> pShader(hShader, ezResourceAcquireMode::NoFallback);

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
  sShaderFile.AppendPath(GetPlatform().GetData());
  sShaderFile.AppendPath(pShader->GetResourceID().GetData());
  sShaderFile.ChangeFileExtension("");
  if (sShaderFile.EndsWith("."))
    sShaderFile.Shrink(0, 1);
  sShaderFile.AppendFormat("%08X.permutation", uiPermutationHash);

  ezShaderPermutationResourceHandle hShaderPermutation = ezResourceManager::GetResourceHandle<ezShaderPermutationResource>(sShaderFile.GetData());

  ezResourceManager::PreloadResource(hShaderPermutation, tShouldBeAvailableIn);

  return hShaderPermutation;
}

void ezShaderManager::ContextEventHandler(ezGALContext::ezGALContextEvent& ed)
{
  switch (ed.m_EventType)
  {
  case ezGALContext::ezGALContextEvent::BeforeDrawcall:
    {
      ContextState& state = s_ContextState[ed.m_pContext];

      // if the shader system is currently in an invalid state, prevent any drawcalls
      SetContextState(ed.m_pContext, state);

      if (!state.m_bStateValid)
        ed.m_bCancelDrawcall = true;
    }
    break;

  default:
    return;
  }
}

void ezShaderManager::SetPlatform(const char* szPlatform, ezGALDevice* pDevice, bool bEnableRuntimeCompilation)
{
  s_pDevice = pDevice;

  ezGALContext::s_ContextEvents.AddEventHandler(ContextEventHandler);

  ezStringBuilder s = szPlatform;
  s.ToUpper();

  s_bEnableRuntimeCompilation = bEnableRuntimeCompilation;
  s_sPlatform = s;

  /// \todo This is a bit hardcoded...
  s_AllowedPermutations.ReadFromFile("ShaderPermutations.txt", s.GetData());

  s_ContextState.Clear();

  // initialize all permutation variables
  for (auto it = s_AllowedPermutations.GetPermutationSet().GetIterator(); it.IsValid(); ++it)
  {
    SetPermutationVariable(it.Key().GetData(), it.Value().GetIterator().Key().GetData());
  }
}

void ezShaderManager::SetPermutationVariable(const char* szVariable, const char* szValue, ezGALContext* pContext)
{
  if (pContext == nullptr)
    pContext = s_pDevice->GetPrimaryContext();

  //ezLog::Debug("Setting '%s' to '%s'", szVariable, szValue);

  ContextState& state = s_ContextState[pContext];

  ezStringBuilder sVar, sVal;
  sVar = szVariable;
  sVal = szValue;

  sVar.ToUpper();
  sVal.ToUpper();

  /// \todo This cannot be checked here anymore, once permutation vars are read from the shader
  if (!s_AllowedPermutations.IsValueAllowed(sVar.GetData(), sVal.GetData()))
  {
    ezLog::Error("Invalid Shader Permutation: '%s' cannot be set to value '%s'", sVar.GetData(), sVal.GetData());
    return;
  }

  auto itVar = state.m_PermutationVariables.FindOrAdd(sVar);

  if (itVar.Value() != sVal)
    state.m_bStateChanged = true;

  itVar.Value() = sVal;
}

void ezShaderManager::SetActiveShader(ezShaderResourceHandle hShader, ezGALContext* pContext)
{
  if (pContext == nullptr)
    pContext = s_pDevice->GetPrimaryContext();

  ContextState& state = s_ContextState[pContext];

  if (state.m_hActiveShader != hShader)
    state.m_bStateChanged = true;

  state.m_hActiveShader = hShader;
}

ezGALShaderHandle ezShaderManager::GetActiveGALShader(ezGALContext* pContext)
{
  if (pContext == nullptr)
    pContext = s_pDevice->GetPrimaryContext();

  ContextState& state = s_ContextState[pContext];

  // make sure the internal state is up to date
  SetContextState(pContext, state);

  if (!state.m_bStateValid)
    return ezGALShaderHandle(); // invalid handle

  return state.m_hActiveGALShader;
}

void ezShaderManager::SetContextState(ezGALContext* pContext, ContextState& state)
{
  if (!state.m_bStateChanged)
    return;

  state.m_bStateChanged = false;
  state.m_bStateValid = false;

  if (!state.m_hActiveShader.IsValid())
    return;

  ezResourceLock<ezShaderResource> pShader(state.m_hActiveShader, ezResourceAcquireMode::AllowFallback);

  state.m_PermGenerator.Clear();
  for (auto itPerm = state.m_PermutationVariables.GetIterator(); itPerm.IsValid(); ++itPerm)
    state.m_PermGenerator.AddPermutation(itPerm.Key().GetData(), itPerm.Value().GetData());

  state.m_PermGenerator.RemoveUnusedPermutations(pShader->GetUsedPermutationVars());

  EZ_ASSERT(state.m_PermGenerator.GetPermutationCount() == 1, "Invalid shader setup");

  ezHybridArray<ezPermutationGenerator::PermutationVar, 16> UsedPermVars;
  state.m_PermGenerator.GetPermutation(0, UsedPermVars);

  ezShaderPermutationResourceHandle hShaderPermutation = PreloadSinglePermutation(state.m_hActiveShader, UsedPermVars, ezTime::Seconds(0.0));

  ezResourceLock<ezShaderPermutationResource> pShaderPermutation(hShaderPermutation, ezResourceAcquireMode::AllowFallback);

  if (!pShaderPermutation->IsShaderValid())
    return;

  state.m_hActiveGALShader = pShaderPermutation->GetGALShader();

  pContext->SetShader(state.m_hActiveGALShader);
  pContext->SetVertexDeclaration(pShaderPermutation->GetGALVertexDeclaration());

  state.m_bStateValid = true;
}



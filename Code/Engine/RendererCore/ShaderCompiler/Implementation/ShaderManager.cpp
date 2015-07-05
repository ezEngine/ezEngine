#include <RendererCore/PCH.h>
#include <RendererCore/Declarations.h>
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
  auto itVar = m_PermutationVariables.FindOrAdd(sVar, &bExisted);

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
  {
    m_StateFlags.Add(ezRenderContextFlags::ShaderStateChanged);

    itVar.Value() = sVal;
  }
}

void ezRenderContext::BindShader(ezShaderResourceHandle hShader, ezBitflags<ezShaderBindFlags> flags)
{
  m_ShaderBindFlags = flags;

  if (flags.IsAnySet(ezShaderBindFlags::ForceRebind) || m_hActiveShader != hShader)
    m_StateFlags.Add(ezRenderContextFlags::ShaderStateChanged);

  m_hActiveShader = hShader;
}

ezGALShaderHandle ezRenderContext::GetActiveGALShader()
{
  // make sure the internal state is up to date
  if (ApplyContextStates(false).Failed())
  {
    // return invalid handle ?
  }

  if (!m_StateFlags.IsSet(ezRenderContextFlags::ShaderStateValid))
    return ezGALShaderHandle(); // invalid handle

  return m_hActiveGALShader;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderManager);


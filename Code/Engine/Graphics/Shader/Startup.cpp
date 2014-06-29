#include <Graphics/PCH.h>
#include <Graphics/Shader/ShaderStageBinary.h>
#include <Graphics/ShaderCompiler/ShaderManager.h>
#include <Foundation/Configuration/Startup.h>

ezPermutationGenerator ezShaderManager::s_AllowedPermutations;
ezMap<ezGALContext*, ezShaderManager::ContextState> ezShaderManager::s_ContextState;
ezString ezShaderManager::s_sPlatform;
ezGALDevice* ezShaderManager::s_pDevice = nullptr;
bool ezShaderManager::s_bEnableRuntimeCompilation = false;
ezString ezShaderManager::s_ShaderCacheDirectory = "ShaderBins";
ezMap<ezUInt32, ezPermutationGenerator> ezShaderManager::s_PermutationHashCache;

void ezShaderManager::OnEngineShutdown()
{
  s_ContextState.Clear();
  s_PermutationHashCache.Clear();
  s_AllowedPermutations.Clear();
}

void ezShaderManager::OnCoreShutdown()
{
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; ++i)
    ezShaderStageBinary::s_ShaderStageBinaries[i].Clear();
}

EZ_BEGIN_SUBSYSTEM_DECLARATION(Graphics, ShaderManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
  }
 
  ON_CORE_SHUTDOWN
  {
    ezShaderManager::OnCoreShutdown();
  }

  ON_ENGINE_STARTUP
  {
  }
 
  ON_ENGINE_SHUTDOWN
  {
    ezShaderManager::OnEngineShutdown();
  }
 
EZ_END_SUBSYSTEM_DECLARATION


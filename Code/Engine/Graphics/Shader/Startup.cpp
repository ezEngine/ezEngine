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

void ezShaderManager::OnEngineShutdown()
{
  s_ContextState.Clear();

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
  }

  ON_ENGINE_STARTUP
  {
  }
 
  ON_ENGINE_SHUTDOWN
  {
    ezShaderManager::OnEngineShutdown();
  }
 
EZ_END_SUBSYSTEM_DECLARATION


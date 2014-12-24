#include <RendererCore/PCH.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererCore/RendererCore.h>
#include <Foundation/Configuration/Startup.h>

ezPermutationGenerator ezRendererCore::s_AllowedPermutations;
ezString ezRendererCore::s_sPlatform;
bool ezRendererCore::s_bEnableRuntimeCompilation = false;
ezString ezRendererCore::s_ShaderCacheDirectory = "ShaderBins";
ezMap<ezUInt32, ezPermutationGenerator> ezRendererCore::s_PermutationHashCache;

void ezRendererCore::OnEngineShutdown()
{
  s_ContextState.Clear();
  s_PermutationHashCache.Clear();
  s_AllowedPermutations.Clear();
}

void ezRendererCore::OnCoreShutdown()
{
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; ++i)
    ezShaderStageBinary::s_ShaderStageBinaries[i].Clear();
}

EZ_BEGIN_SUBSYSTEM_DECLARATION(Graphics, RendererCore)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
  }
 
  ON_CORE_SHUTDOWN
  {
    ezRendererCore::OnCoreShutdown();
  }

  ON_ENGINE_STARTUP
  {
  }
 
  ON_ENGINE_SHUTDOWN
  {
    ezRendererCore::OnEngineShutdown();
  }
 
EZ_END_SUBSYSTEM_DECLARATION


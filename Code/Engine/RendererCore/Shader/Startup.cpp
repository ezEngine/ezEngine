#include <RendererCore/PCH.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererCore/RendererCore.h>
#include <Foundation/Configuration/Startup.h>

ezPermutationGenerator ezRendererCore::s_AllowedPermutations;
ezString ezRendererCore::s_sPlatform;
ezString ezRendererCore::s_sPermVarSubDir;
bool ezRendererCore::s_bEnableRuntimeCompilation = false;
ezString ezRendererCore::s_ShaderCacheDirectory = "ShaderBins";
ezMap<ezUInt32, ezPermutationGenerator> ezRendererCore::s_PermutationHashCache;
ezMap<ezRendererCore::ShaderVertexDecl, ezGALVertexDeclarationHandle> ezRendererCore::s_GALVertexDeclarations;

void ezRendererCore::OnEngineShutdown()
{
  ezShaderStageBinary::OnEngineShutdown();

  s_ContextState.Clear();
  s_PermutationHashCache.Clear();
  s_AllowedPermutations.Clear();
  s_hGlobalConstantBuffer.Invalidate();

  for (auto it = s_GALVertexDeclarations.GetIterator(); it.IsValid(); ++it)
  {
    ezGALDevice::GetDefaultDevice()->DestroyVertexDeclaration(it.Value());
  }

  s_GALVertexDeclarations.Clear();

  // reset to a default state by re-constructing the struct
  ezMemoryUtils::Construct(&s_GlobalConstants, 1);
}

void ezRendererCore::OnCoreShutdown()
{
  EZ_ASSERT_DEV(s_GALVertexDeclarations.IsEmpty(), "ezRendererCore::OnEngineShutdown has not been called. Either ezStartup::ShutdownEngine was not called or ezStartup::StartupEngine was not called at program start");

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



EZ_STATICLINK_FILE(RendererCore, RendererCore_Shader_Startup);


#include <RendererCore/PCH.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/Configuration/Startup.h>

ezPermutationGenerator ezRenderContext::s_AllowedPermutations;
ezString ezRenderContext::s_sPlatform;
ezString ezRenderContext::s_sPermVarSubDir;
bool ezRenderContext::s_bEnableRuntimeCompilation = false;
ezString ezRenderContext::s_ShaderCacheDirectory = "ShaderCache";
ezMap<ezUInt32, ezPermutationGenerator> ezRenderContext::s_PermutationHashCache;
ezMap<ezRenderContext::ShaderVertexDecl, ezGALVertexDeclarationHandle> ezRenderContext::s_GALVertexDeclarations;

void ezRenderContext::OnEngineShutdown()
{
  ezShaderStageBinary::OnEngineShutdown();

  for (auto rc : s_Instances)
    EZ_DEFAULT_DELETE(rc);
  
  s_Instances.Clear();
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

void ezRenderContext::OnCoreShutdown()
{
  EZ_ASSERT_DEV(s_GALVertexDeclarations.IsEmpty(), "ezRenderContext::OnEngineShutdown has not been called. Either ezStartup::ShutdownEngine was not called or ezStartup::StartupEngine was not called at program start");

  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; ++i)
    ezShaderStageBinary::s_ShaderStageBinaries[i].Clear();
}

EZ_BEGIN_SUBSYSTEM_DECLARATION(Graphics, RendererContext)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
  }
 
  ON_CORE_SHUTDOWN
  {
    ezRenderContext::OnCoreShutdown();
  }

  ON_ENGINE_STARTUP
  {
  }
 
  ON_ENGINE_SHUTDOWN
  {
    ezRenderContext::OnEngineShutdown();
  }
 
EZ_END_SUBSYSTEM_DECLARATION



EZ_STATICLINK_FILE(RendererCore, RendererCore_Shader_Startup);


#include <RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, ReflectionPool)

BEGIN_SUBSYSTEM_DEPENDENCIES
"Foundation",
"Core",
"RenderWorld"
END_SUBSYSTEM_DEPENDENCIES

ON_HIGHLEVELSYSTEMS_STARTUP
{
  ezReflectionPool::OnEngineStartup();
}

ON_HIGHLEVELSYSTEMS_SHUTDOWN
{
  ezReflectionPool::OnEngineShutdown();
}

EZ_END_SUBSYSTEM_DECLARATION;
  // clang-format on

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
//ezCVarBool CVarShadowPoolStats("r_ShadowPoolStats", false, ezCVarFlags::Default, "Display same stats of the shadow pool");
#endif

namespace
{
  
} // namespace

// static
void ezReflectionPool::AddReflectionProbe(const ezReflectionProbeData& data, float fPriority /*= 0.0f*/) {}

// static
void ezReflectionPool::OnEngineStartup()
{
  //s_pPool = EZ_DEFAULT_NEW(PoolData);

  ezRenderWorld::s_EndExtractionEvent.AddEventHandler(OnEndExtraction);
}

// static
void ezReflectionPool::OnEngineShutdown()
{
  ezRenderWorld::s_EndExtractionEvent.RemoveEventHandler(OnEndExtraction);

  //EZ_DEFAULT_DELETE(s_pPool);
}

// static
void ezReflectionPool::OnEndExtraction(ezUInt64 uiFrameCounter)
{
  EZ_PROFILE_SCOPE("Reflection Pool Update");

  
}

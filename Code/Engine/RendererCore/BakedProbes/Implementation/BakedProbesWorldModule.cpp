#include <RendererCorePCH.h>

#include <RendererCore/BakedProbes/BakedProbesWorldModule.h>

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezBakedProbesWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBakedProbesWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

ezBakedProbesWorldModule::ezBakedProbesWorldModule(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{
}

ezBakedProbesWorldModule::~ezBakedProbesWorldModule() = default;

void ezBakedProbesWorldModule::Initialize()
{
}

void ezBakedProbesWorldModule::Deinitialize()
{
}

ezResult ezBakedProbesWorldModule::GetProbeIndexData(const ezVec3& globalPosition, const ezVec3& normal, ProbeIndexData& out_ProbeIndexData) const
{
  return EZ_FAILURE;
}

ezAmbientCube<float> ezBakedProbesWorldModule::GetSkyVisibility(const ProbeIndexData& indexData) const
{
  return ezAmbientCube<float>();
}

#include <RecastPluginPCH.h>

#include <Core/World/World.h>
#include <Recast/DetourCrowd.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezRecastWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRecastWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRecastWorldModule::ezRecastWorldModule(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{
}

ezRecastWorldModule::~ezRecastWorldModule() = default;

void ezRecastWorldModule::Initialize()
{
  {
    auto updateDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezRecastWorldModule::UpdateNavMesh, this);
    updateDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;
    updateDesc.m_fPriority = 0.0f;

    RegisterUpdateFunction(updateDesc);
  }
}

void ezRecastWorldModule::SetNavMeshResource(const ezRecastNavMeshResourceHandle& hNavMesh)
{
  m_hNavMesh = hNavMesh;
  m_pDetourNavMesh = nullptr;
  m_pNavMeshPointsOfInterest.Clear();

  if (hNavMesh.IsValid())
  {
    ezResourceLock<ezRecastNavMeshResource> pNavMesh(hNavMesh, ezResourceAcquireMode::NoFallback);
    m_pDetourNavMesh = pNavMesh->GetNavMesh();

    m_pNavMeshPointsOfInterest = EZ_DEFAULT_NEW(ezNavMeshPointOfInterestGraph);
    m_pNavMeshPointsOfInterest->ExtractInterestPointsFromMesh(*pNavMesh->GetNavMeshPolygons());
  }
}

void ezRecastWorldModule::UpdateNavMesh(const UpdateContext& ctxt)
{
  if (m_pNavMeshPointsOfInterest)
  {
    m_pNavMeshPointsOfInterest->IncreaseCheckVisibiblityTimeStamp(GetWorld()->GetClock().GetAccumulatedTime());
  }
}

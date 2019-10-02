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
  SUPER::Initialize();

  {
    auto updateDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezRecastWorldModule::UpdateNavMesh, this);
    updateDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    updateDesc.m_bOnlyUpdateWhenSimulating = false;
    updateDesc.m_fPriority = 0.0f;

    RegisterUpdateFunction(updateDesc);
  }

  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezRecastWorldModule::ResourceEventHandler, this));
}

void ezRecastWorldModule::Deinitialize()
{
  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezRecastWorldModule::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void ezRecastWorldModule::SetNavMeshResource(const ezRecastNavMeshResourceHandle& hNavMesh)
{
  m_hNavMesh = hNavMesh;
  m_pDetourNavMesh = nullptr;
  m_pNavMeshPointsOfInterest.Clear();
}

void ezRecastWorldModule::UpdateNavMesh(const UpdateContext& ctxt)
{
  if (m_pDetourNavMesh == nullptr && m_hNavMesh.IsValid())
  {
    ezResourceLock<ezRecastNavMeshResource> pNavMesh(m_hNavMesh, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pNavMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
      return;

    m_pDetourNavMesh = pNavMesh->GetNavMesh();

    m_pNavMeshPointsOfInterest = EZ_DEFAULT_NEW(ezNavMeshPointOfInterestGraph);
    m_pNavMeshPointsOfInterest->ExtractInterestPointsFromMesh(*pNavMesh->GetNavMeshPolygons());
  }

  if (m_pNavMeshPointsOfInterest)
  {
    m_pNavMeshPointsOfInterest->IncreaseCheckVisibiblityTimeStamp(GetWorld()->GetClock().GetAccumulatedTime());
  }
}

void ezRecastWorldModule::ResourceEventHandler(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUnloading &&
      e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezRecastNavMeshResource>())
  {
    // triggers a recreation in the next update
    m_pDetourNavMesh = nullptr;
  }
}

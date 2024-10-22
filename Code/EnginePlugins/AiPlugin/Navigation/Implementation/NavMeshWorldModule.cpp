#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshWorldModule.h>
#include <Core/Interfaces/NavmeshGeoWorldModule.h>
#include <Core/World/World.h>
#include <DetourNavMesh.h>
#include <Foundation/Configuration/CVar.h>

ezCVarInt cvar_NavMeshVisualize("AI.Navmesh.Visualize", -1, ezCVarFlags::None, "Visualize the n-th navmesh.");

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezAiNavMeshWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiNavMeshWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAiNavMeshWorldModule::ezAiNavMeshWorldModule(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{
  m_Config.Load().IgnoreResult();

  {
    // add a default filter
    auto& cfg = m_Config.m_PathSearchConfigs.ExpandAndGetRef();
  }

  for (const auto& cfg : m_Config.m_PathSearchConfigs)
  {
    auto& filter = m_PathSearchFilters[cfg.m_sName];

    ezUInt32 groundMask = 0;
    for (ezUInt32 gt = 0; gt < ezAiNumGroundTypes; ++gt)
    {
      if (cfg.m_bGroundTypeAllowed[gt])
        groundMask |= (1 << gt);

      filter.setAreaCost((int)gt, cfg.m_fGroundTypeCost[gt]);
    }

    filter.setIncludeAreaBits(groundMask);
  }

  if (m_Config.m_NavmeshConfigs.IsEmpty())
  {
    // insert a default navmesh config
    m_Config.m_NavmeshConfigs.ExpandAndGetRef();
  }
}

ezAiNavMeshWorldModule::~ezAiNavMeshWorldModule()
{
  for (const auto& cfg : m_Config.m_NavmeshConfigs)
  {
    EZ_DEFAULT_DELETE(m_WorldNavMeshes[cfg.m_sName]);
  }
}

void ezAiNavMeshWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezAiNavMeshWorldModule::Update, this);
    updateDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostTransform;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(updateDesc);
  }

  m_WorldNavMeshes.Clear();

  for (const auto& cfg : m_Config.m_NavmeshConfigs)
  {
    // TODO: make tile size etc configurable
    m_WorldNavMeshes[cfg.m_sName] = EZ_DEFAULT_NEW(ezAiNavMesh, cfg);
  }

  m_pGenerateSectorTask = EZ_DEFAULT_NEW(ezNavMeshSectorGenerationTask);
  m_pGenerateSectorTask->ConfigureTask("Generate Navmesh Sector", ezTaskNesting::Maybe);
}

void ezAiNavMeshWorldModule::Deinitialize()
{
  m_pGenerateSectorTask = nullptr;
  ezTaskSystem::CancelGroup(m_GenerateSectorTaskID).IgnoreResult();
  ezTaskSystem::WaitForGroup(m_GenerateSectorTaskID);
}

ezAiNavMesh* ezAiNavMeshWorldModule::GetNavMesh(ezStringView sName)
{
  auto it = m_WorldNavMeshes.Find(sName);
  if (it.IsValid())
    return it.Value();

  return nullptr;
}

const ezAiNavMesh* ezAiNavMeshWorldModule::GetNavMesh(ezStringView sName) const
{
  auto it = m_WorldNavMeshes.Find(sName);
  if (it.IsValid())
    return it.Value();

  return nullptr;
}

void ezAiNavMeshWorldModule::Update(const UpdateContext& ctxt)
{
  if (m_uiUpdateDelay > 0)
  {
    --m_uiUpdateDelay;
    return;
  }

  for (auto& nm : m_WorldNavMeshes)
  {
    nm.Value()->FinalizeSectorUpdates();
  }

  if (cvar_NavMeshVisualize >= 0)
  {
    ezInt32 i = cvar_NavMeshVisualize;
    for (auto it = m_WorldNavMeshes.GetIterator(); it.IsValid(); ++it)
    {
      if (i-- == 0)
      {
        it.Value()->DebugDraw(GetWorld(), m_Config);
        break;
      }
    }
  }

  if (!ezTaskSystem::IsTaskGroupFinished(m_GenerateSectorTaskID))
    return;

  auto pNavGeo = GetWorld()->GetOrCreateModule<ezNavmeshGeoWorldModuleInterface>();
  if (pNavGeo == nullptr)
    return;

  for (auto& nm : m_WorldNavMeshes)
  {
    auto sectorID = nm.Value()->RetrieveRequestedSector();
    if (sectorID == ezInvalidIndex)
      continue;

    m_pGenerateSectorTask->m_pWorldNavMesh = nm.Value();
    m_pGenerateSectorTask->m_SectorID = sectorID;
    m_pGenerateSectorTask->m_pNavGeo = pNavGeo;

    m_GenerateSectorTaskID = ezTaskSystem::StartSingleTask(m_pGenerateSectorTask, ezTaskPriority::LongRunning);

    break;
  }
}

const dtQueryFilter& ezAiNavMeshWorldModule::GetPathSearchFilter(ezStringView sName) const
{
  auto it = m_PathSearchFilters.Find(sName);
  if (it.IsValid())
    return it.Value();

  it = m_PathSearchFilters.Find("");
  ezLog::Warning("Ai Path Search Filter '{}' does not exist.", sName);
  return it.Value();
}


EZ_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation_Implementation_NavMeshWorldModule);


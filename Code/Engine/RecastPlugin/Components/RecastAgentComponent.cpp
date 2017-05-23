#include <PCH.h>
#include <RecastPlugin/Components/RecastAgentComponent.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>
#include <ThirdParty/Recast/DetourCrowd.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezRcAgentComponent, 1)
//{
//  EZ_BEGIN_PROPERTIES
//  {
//  }
//  EZ_END_PROPERTIES
//}
EZ_END_COMPONENT_TYPE

ezRcAgentComponent::ezRcAgentComponent() { }
ezRcAgentComponent::~ezRcAgentComponent() { }

void ezRcAgentComponent::SerializeComponent(ezWorldWriter& stream) const
{

}

void ezRcAgentComponent::DeserializeComponent(ezWorldReader& stream)
{

}

static float frand()
{
  //	return ((float)(rand() & 0xffff)/(float)0xffff);
  return (float)rand() / (float)RAND_MAX;
}

void ezRcAgentComponent::Update()
{
  if (!IsActiveAndSimulating())
    return;

  if (m_iAgentIndex == -1)
  {
    if (!GetManager()->GetRecastWorldModule()->IsInitialized())
      return;

    dtCrowd* pCrowd = GetManager()->GetRecastWorldModule()->m_pCrowd;

    ezVec3 pos = GetOwner()->GetGlobalPosition();
    ezMath::Swap(pos.y, pos.z); // other coord system in recast

    dtCrowdAgentParams params;
    params.height = 1.5f;
    params.radius = 0.3f;
    params.maxAcceleration = 20.0f;
    params.maxSpeed = 5.0f;
    params.collisionQueryRange = 10.0f;
    params.pathOptimizationRange = 15.0f;
    params.separationWeight = 2.0f;
    params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OBSTACLE_AVOIDANCE | DT_CROWD_SEPARATION | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO;
    params.obstacleAvoidanceType = 0;
    params.queryFilterType = 0;
    params.userData = nullptr;

    m_iAgentIndex = pCrowd->addAgent(&pos.x, &params);

    return;
  }

  if (m_iAgentIndex != -1)
  {
    dtCrowd* pCrowd = GetManager()->GetRecastWorldModule()->m_pCrowd;

    const dtCrowdAgent* pAgent = pCrowd->getAgent(m_iAgentIndex);

    ezVec3 newPos(pAgent->npos[0], pAgent->npos[2], pAgent->npos[1]);

    GetOwner()->SetGlobalPosition(newPos);

    if (GetWorld()->GetClock().GetAccumulatedTime() - m_tLastUpdate > ezTime::Seconds(5.0f))
    {
      m_tLastUpdate = GetWorld()->GetClock().GetAccumulatedTime();

      dtQueryFilter filter;
      dtPolyRef ref;
      float pt[3];
      if (dtStatusFailed(pCrowd->getNavMeshQuery()->findRandomPoint(&filter, frand, &ref, pt)))
      {
        ezLog::Error("Could not find random point");
      }
      else
      {
        pCrowd->requestMoveTarget(m_iAgentIndex, ref, pt);
      }
    }
  }
}

void ezRcAgentComponent::OnSimulationStarted()
{
  m_iAgentIndex = -1;
  m_tLastUpdate = GetWorld()->GetClock().GetAccumulatedTime();
}

//////////////////////////////////////////////////////////////////////////

ezRcAgentComponentManager::ezRcAgentComponentManager(ezWorld* pWorld) : SUPER(pWorld) { }
ezRcAgentComponentManager::~ezRcAgentComponentManager() { }

void ezRcAgentComponentManager::Initialize()
{
  SUPER::Initialize();

  // make sure this world module exists
  m_pWorldModule = GetWorld()->GetOrCreateModule<ezRecastWorldModule>();

  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezRcAgentComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void ezRcAgentComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActive())
      it->Update();
  }
}

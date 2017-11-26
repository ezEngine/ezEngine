#include <PCH.h>
#include <RecastPlugin/Components/SoldierComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <GameEngine/Components/AgentSteeringComponent.h>
#include <ThirdParty/Recast/DetourCrowd.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezSoldierComponent, 1, ezComponentMode::Dynamic)
{
  //EZ_BEGIN_PROPERTIES
  //{
  //}
  //EZ_END_PROPERTIES
}
EZ_END_COMPONENT_TYPE

ezSoldierComponent::ezSoldierComponent() { }
ezSoldierComponent::~ezSoldierComponent() { }

void ezSoldierComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

}

void ezSoldierComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

}

void ezSoldierComponent::OnSimulationStarted()
{
  ezAgentSteeringComponent* pSteering = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType<ezAgentSteeringComponent>(pSteering))
  {
    m_hSteeringComponent = pSteering->GetHandle();

    pSteering->m_SteeringEvents.AddEventHandler(ezMakeDelegate(&ezSoldierComponent::SteeringEventHandler, this));
  }

  m_State = State::Idle;
}

void ezSoldierComponent::Deinitialize()
{
  ezAgentSteeringComponent* pSteering;
  if (GetWorld()->TryGetComponent(m_hSteeringComponent, pSteering))
  {
    if (pSteering->m_SteeringEvents.HasEventHandler(ezMakeDelegate(&ezSoldierComponent::SteeringEventHandler, this)))
    {
      pSteering->m_SteeringEvents.RemoveEventHandler(ezMakeDelegate(&ezSoldierComponent::SteeringEventHandler, this));
    }
  }

  SUPER::Deinitialize();
}

static float frand()
{
  return (float)rand() / (float)RAND_MAX;
}

void ezSoldierComponent::Update()
{
  if (!IsActiveAndSimulating())
    return;

  if (m_State == State::Idle)
  {
    ezRecastWorldModule* pRecastModule = GetWorld()->GetOrCreateModule<ezRecastWorldModule>();

    if (pRecastModule == nullptr || pRecastModule->m_pCrowd == nullptr)
      return;

    ezAgentSteeringComponent* pSteering = nullptr;
    if (!GetWorld()->TryGetComponent(m_hSteeringComponent, pSteering))
      return;


    ezVec3 vNewTargetPos;
    bool bFoundAny = false;

    {
      dtCrowd* pCrowd = pRecastModule->m_pCrowd;

      dtQueryFilter filter;
      dtPolyRef ref;
      float pt[3];
      if (dtStatusFailed(pCrowd->getNavMeshQuery()->findRandomPoint(&filter, frand, &ref, pt)))
      {
        ezLog::Error("Could not find random point");
      }
      else
      {
        vNewTargetPos = ezVec3(pt[0], pt[2], pt[1]);
      }
    }

    {
      const auto& graph = pRecastModule->m_NavMeshPointsOfInterest.GetGraph();

      const ezUInt32 uiTimestamp = pRecastModule->m_NavMeshPointsOfInterest.GetCheckVisibilityTimeStamp() - 10;

      const ezVec3 vOwnPos = GetOwner()->GetGlobalPosition();

      ezDynamicArray<ezUInt32> points;
      graph.FindPointsOfInterest(vOwnPos, 10.0, points);

      float fBestDistance = 1000;

      for (ezUInt32 i = 0; i < points.GetCount(); ++i)
      {
        const ezUInt32 marker = graph.GetPoints()[points[i]].m_uiVisibleMarker;

        const bool bHalfVisible = (marker >= uiTimestamp) && ((marker & 3U) == 2);
        const bool bInvisible = (marker < uiTimestamp) || ((marker & 3U) == 0);

        const ezVec3 ptPos = graph.GetPoints()[points[i]].m_vFloorPosition;
        const float fDist = (vOwnPos - ptPos).GetLength();

        if (bHalfVisible) // top visible, bottom invisible
        {
          if (fDist < fBestDistance)
          {
            bFoundAny = true;
            fBestDistance = fDist;
            vNewTargetPos = ptPos;
          }
        }

        if (bInvisible)
        {
          if (fDist * 2 < fBestDistance)
          {
            bFoundAny = true;
            fBestDistance = fDist * 2;
            vNewTargetPos = ptPos;
          }
        }
      }
    }

    if (bFoundAny)
    {
      pSteering->SetTargetPosition(vNewTargetPos);
      m_State = State::WaitingForPath;
    }
  }
}

void ezSoldierComponent::SteeringEventHandler(const ezAgentSteeringEvent& e)
{
  switch (e.m_Type)
  {
  case ezAgentSteeringEvent::TargetReached:
  case ezAgentSteeringEvent::TargetCleared:
  case ezAgentSteeringEvent::ErrorInvalidTargetPosition:
  case ezAgentSteeringEvent::ErrorNoPathToTarget:
  case ezAgentSteeringEvent::WarningNoFullPathToTarget:
    {
      e.m_pComponent->ClearTargetPosition();
      m_State = State::Idle;
    }
    break;

  case ezAgentSteeringEvent::PathToTargetFound:
    {
      m_State = State::Walking;
    }
    break;

  case ezAgentSteeringEvent::ErrorOutsideNavArea:
  case ezAgentSteeringEvent::ErrorSteeringFailed:
    {
      EZ_ASSERT_DEV(m_State != State::ErrorState, "Multi-error state?");

      e.m_pComponent->ClearTargetPosition();

      m_State = State::ErrorState;
      ezLog::Error("NPC is now in error state");
    }
    break;
  }
}


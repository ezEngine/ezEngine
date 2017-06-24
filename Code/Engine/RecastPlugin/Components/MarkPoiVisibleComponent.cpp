#include <PCH.h>
#include <RecastPlugin/Components/MarkPoiVisibleComponent.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezRcMarkPoiVisibleComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(20.0f)),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
  }
  EZ_END_PROPERTIES
}
EZ_END_COMPONENT_TYPE

ezRcMarkPoiVisibleComponent::ezRcMarkPoiVisibleComponent() { }
ezRcMarkPoiVisibleComponent::~ezRcMarkPoiVisibleComponent() { }

void ezRcMarkPoiVisibleComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_fRadius;
  s << m_uiCollisionLayer;
}

void ezRcMarkPoiVisibleComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_fRadius;
  s >> m_uiCollisionLayer;
}

void ezRcMarkPoiVisibleComponent::Update()
{
  if (!IsActiveAndSimulating() || m_pWorldModule == nullptr)
    return;

  if (m_pPhysicsModule == nullptr)
  {
    m_pPhysicsModule = GetWorld()->GetModuleOfBaseType<ezPhysicsWorldModuleInterface>();

    if (m_pPhysicsModule == nullptr)
      return;
  }

  const ezVec3 vOwnPos = GetOwner()->GetGlobalPosition();

  auto& graph = m_pWorldModule->m_NavMeshPointsOfInterest.GetGraph();
  auto& POIs = graph.AccessPoints();

  ezDynamicArray<ezUInt32> points(ezFrameAllocator::GetCurrentAllocator());
  graph.FindPointsOfInterest(vOwnPos, 20.0f, points);

  for (ezUInt32 i = 0; i < points.GetCount(); ++i)
  {
    auto& poi = POIs[points[i]];
    const ezVec3 vTarget = poi.m_vFloorPosition + ezVec3(0, 0, 0.5f);
    const ezVec3 vDir = vTarget - vOwnPos;
    const float fRayLen = vDir.GetLength();
    const ezVec3 vDirNorm = vDir / fRayLen;

    ezPhysicsHitResult hit;
    const bool bHit = m_pPhysicsModule->CastRay(vOwnPos, vDirNorm, fRayLen, m_uiCollisionLayer, hit);

    if (bHit)
    {
      poi.m_uiVisibleMarkerLow = 0xffffffff;
    }
    else
    {
      poi.m_uiVisibleMarkerLow = 0;
    }
  }
}

void ezRcMarkPoiVisibleComponent::OnSimulationStarted()
{
  m_pWorldModule = GetWorld()->GetOrCreateModule<ezRecastWorldModule>();
  
}


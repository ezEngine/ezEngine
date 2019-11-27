#include <RecastPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <RecastPlugin/Components/MarkPoiVisibleComponent.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRcMarkPoiVisibleComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(20.0f)),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezRcMarkPoiVisibleComponent::ezRcMarkPoiVisibleComponent() = default;
ezRcMarkPoiVisibleComponent::~ezRcMarkPoiVisibleComponent() = default;

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
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_fRadius;
  s >> m_uiCollisionLayer;
}

static const ezInt32 g_iMaxPointsToCheckPerFrame = 30;

void ezRcMarkPoiVisibleComponent::Update()
{
  if (!IsActiveAndSimulating() || m_pWorldModule == nullptr)
    return;

  if (m_pPhysicsModule == nullptr)
  {
    m_pPhysicsModule = GetWorld()->GetModule<ezPhysicsWorldModuleInterface>();

    if (m_pPhysicsModule == nullptr)
      return;
  }

  const ezVec3 vOwnPos = GetOwner()->GetGlobalPosition();

  auto pPoiGraph = m_pWorldModule->AccessNavMeshPointsOfInterestGraph();

  if (pPoiGraph == nullptr)
    return;

  const ezUInt32 uiCheckTimeStamp = pPoiGraph->GetCheckVisibilityTimeStamp();
  const ezUInt32 uiTimeStampFullyVisible = uiCheckTimeStamp | 3U;
  const ezUInt32 uiTimeStampTopVisible = uiCheckTimeStamp | 2U;

  const ezUInt32 uiSkipCheckTimeStamp = uiCheckTimeStamp - 10;

  auto& graph = pPoiGraph->GetGraph();
  auto& POIs = graph.AccessPoints();

  ezDynamicArray<ezUInt32> points(ezFrameAllocator::GetCurrentAllocator());
  graph.FindPointsOfInterest(vOwnPos, 20.0f, points);

  ezInt32 iPointsToCheck = g_iMaxPointsToCheckPerFrame;

  for (ezUInt32 i = 0; i < points.GetCount(); ++i)
  {
    ++m_uiLastFirstCheckedPoint;

    if (m_uiLastFirstCheckedPoint >= points.GetCount())
      m_uiLastFirstCheckedPoint = 0;

    auto& poi = POIs[points[m_uiLastFirstCheckedPoint]];

    if (poi.m_uiVisibleMarker >= uiSkipCheckTimeStamp)
      continue;

    if (--iPointsToCheck <= 0)
      break;

    const ezVec3 vTargetBottom = poi.m_vFloorPosition + ezVec3(0, 0, 0.5f);
    ezVec3 vDirToBottom = vTargetBottom - vOwnPos;
    const float fRayLenBottom = vDirToBottom.GetLengthAndNormalize();

    ezPhysicsHitResult hit;

    if (m_pPhysicsModule->CastRay(vOwnPos, vDirToBottom, fRayLenBottom, m_uiCollisionLayer, hit, ezPhysicsShapeType::Static))
    {
      const ezVec3 vTargetTop = poi.m_vFloorPosition + ezVec3(0, 0, 1.0f);
      ezVec3 vDirToTop = vTargetTop - vOwnPos;
      const float fRayLenTop = vDirToTop.GetLengthAndNormalize();

      --iPointsToCheck;

      ezPhysicsHitResult hit2;
      if (m_pPhysicsModule->CastRay(vOwnPos, vDirToTop, fRayLenTop, m_uiCollisionLayer, hit2, ezPhysicsShapeType::Static))
      {
        poi.m_uiVisibleMarker = uiCheckTimeStamp;
      }
      else
      {
        poi.m_uiVisibleMarker = uiTimeStampTopVisible;
      }
    }
    else
    {
      poi.m_uiVisibleMarker = uiTimeStampFullyVisible;
    }
  }
}

void ezRcMarkPoiVisibleComponent::OnSimulationStarted()
{
  m_pWorldModule = GetWorld()->GetOrCreateModule<ezRecastWorldModule>();
}

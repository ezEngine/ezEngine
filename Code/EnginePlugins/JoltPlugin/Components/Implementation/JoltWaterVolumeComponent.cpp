#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/Components/JoltWaterVolumeComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>

namespace
{
  class WaterCollector : public JPH::CollideShapeBodyCollector
  {
  public:
    WaterCollector(JPH::PhysicsSystem& system, const ezPlane& surfacePlane, const ezVec3& vFlow, float fDeltaTime)
      : m_system(system)
      , m_surfacePlane(surfacePlane)
      , m_flow(ezJoltConversionUtils::ToVec3(vFlow))
      , m_fDeltaTime(fDeltaTime)
    {
    }

    virtual void AddHit(const JPH::BodyID& inBodyID) override
    {
      JPH::BodyLockWrite lock(m_system.GetBodyLockInterface(), inBodyID);
      JPH::Body& body = lock.GetBody();
      if (body.IsActive() && body.IsDynamic())
      {
        const ezVec3 pos = ezJoltConversionUtils::ToVec3(body.GetCenterOfMassPosition());
        const ezVec3 surfacePosition = m_surfacePlane.ProjectOntoPlane(pos);

        const JPH::Vec3 surfacePositionJolt = ezJoltConversionUtils::ToVec3(surfacePosition);
        const JPH::Vec3 surfaceNormal = ezJoltConversionUtils::ToVec3(m_surfacePlane.m_vNormal);

        body.ApplyBuoyancyImpulse(surfacePositionJolt, surfaceNormal, 1.1f, 0.3f, 0.05f, m_flow, m_system.GetGravity(), m_fDeltaTime);
      }
    }

  private:
    JPH::PhysicsSystem& m_system;
    ezPlane m_surfacePlane;
    JPH::Vec3 m_flow;
    float m_fDeltaTime;
  };
}

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltWaterVolumeComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Extents", m_vExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(10.0f)), new ezClampValueAttribute(ezVec3(0.0f), ezVariant())),
    EZ_MEMBER_PROPERTY("Flow", m_vFlow),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Effects"),
    new ezBoxManipulatorAttribute("Extents", 1.0f, true),
    new ezBoxVisualizerAttribute("Extents", 1.0f, ezColorScheme::GetCategoryColor("Physics", ezColorScheme::CategoryColorUsage::ViewportIcon)),
    new ezDirectionVisualizerAttribute("Flow", 1.0f, ezColorScheme::GetCategoryColor("Physics", ezColorScheme::CategoryColorUsage::ViewportIcon)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezJoltWaterVolumeComponentManager::ezJoltWaterVolumeComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezJoltWaterVolumeComponent, ezBlockStorageType::FreeList>(pWorld)
{
}

ezJoltWaterVolumeComponentManager::~ezJoltWaterVolumeComponentManager() = default;

void ezJoltWaterVolumeComponentManager::UpdateWaterVolumes(ezTime deltaTime)
{
  EZ_PROFILE_SCOPE("UpdateWaterVolumes");

  auto pJoltSystem = GetWorld()->GetModule<ezJoltWorldModule>()->GetJoltSystem();

  for (auto it = GetComponents(0); it.IsValid(); it.Next())
  {
    if (it->IsActiveAndInitialized())
    {
      it->Update(*pJoltSystem, deltaTime);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

ezJoltWaterVolumeComponent::ezJoltWaterVolumeComponent() = default;
ezJoltWaterVolumeComponent::~ezJoltWaterVolumeComponent() = default;

void ezJoltWaterVolumeComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_vFlow;
}

void ezJoltWaterVolumeComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  /*const ezUInt32 uiVersion =*/inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_vFlow;
}

void ezJoltWaterVolumeComponent::Update(JPH::PhysicsSystem& joltSystem, ezTime deltaTime)
{
  const ezTransform globalTransform = GetOwner()->GetGlobalTransform();
  const ezVec3 vScaledExtents = m_vExtents.CompMul(globalTransform.m_vScale);

  if (vScaledExtents.x * vScaledExtents.y * vScaledExtents.z < ezMath::DefaultEpsilon<float>())
    return;

  const ezVec3 gravity = ezJoltConversionUtils::ToVec3(joltSystem.GetGravity());

  if (GetOwner()->IsDynamic() || m_vGravity.IsEqual(gravity, ezMath::DefaultEpsilon<float>()) == false)
  {
    UpdateWaterPlane(gravity);
  }

  const ezVec3 halfExtents = vScaledExtents * 0.5f;
  const ezVec3 flow = globalTransform.TransformDirection(m_vFlow);

  JPH::Vec3 joltHalfExtents = ezJoltConversionUtils::ToVec3(halfExtents);
  JPH::AABox waterBox(-joltHalfExtents, joltHalfExtents);
  waterBox.Translate(ezJoltConversionUtils::ToVec3(globalTransform.m_vPosition));

  WaterCollector collector(joltSystem, m_surfacePlane, flow, deltaTime.AsFloatInSeconds());
  ezJoltBroadPhaseLayerFilter broadphaseFilter(ezPhysicsShapeType::Dynamic);
  // TODO: collision layer

  joltSystem.GetBroadPhaseQuery().CollideAABox(waterBox, collector, broadphaseFilter);
}

void ezJoltWaterVolumeComponent::UpdateWaterPlane(const ezVec3& vGravity)
{
  const ezTransform globalTransform = GetOwner()->GetGlobalTransform();
  const ezVec3 halfExtents = m_vExtents * 0.5f;
  const ezVec3 vGravityDir = vGravity.GetNormalized();
  float fMinDot = 1000.0f;

  for (ezUInt32 i = 0; i< 6; ++i)
  {
    ezVec3 vNormal = ezBasisAxis::GetBasisVector(static_cast<ezBasisAxis::Enum>(i));
    ezVec3 vPoint = vNormal.CompMul(halfExtents);
    vNormal = globalTransform.TransformDirection(vNormal).GetNormalized();

    float fDot = vNormal.Dot(vGravityDir);
    if (fDot < fMinDot)
    {
      vPoint = globalTransform.TransformPosition(vPoint);
      m_surfacePlane = ezPlane::MakeFromNormalAndPoint(vNormal, vPoint);

      fMinDot = fDot;
    }
  }

  m_vGravity = vGravity;
}

EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltWaterVolumeComponent);

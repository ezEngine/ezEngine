#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/Components/JoltWaterVolumeComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltWaterVolumeComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Extents", m_vExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(10.0f)), new ezClampValueAttribute(ezVec3(0.0f), ezVariant())),
    EZ_MEMBER_PROPERTY("LocalFlow", m_vLocalFlow),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Effects"),
    new ezBoxManipulatorAttribute("Extents", 1.0f, true),
    new ezBoxVisualizerAttribute("Extents", 1.0f, ezColorScheme::GetCategoryColor("Physics", ezColorScheme::CategoryColorUsage::ViewportIcon)),
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
  s << m_vLocalFlow;
}

void ezJoltWaterVolumeComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  /*const ezUInt32 uiVersion =*/inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_vLocalFlow;
}

void ezJoltWaterVolumeComponent::Update(JPH::PhysicsSystem& joltSystem, ezTime deltaTime)
{
  class MyCollector : public JPH::CollideShapeBodyCollector
  {
  public:
    MyCollector(JPH::PhysicsSystem& inSystem, const ezVec3& inSurfacePosition, const ezVec3& inSurfaceNormal, float inDeltaTime)
      : m_system(inSystem)
      , mSurfacePosition(ezJoltConversionUtils::ToVec3(inSurfacePosition))
      , mSurfaceNormal(ezJoltConversionUtils::ToVec3(inSurfaceNormal))
      , mDeltaTime(inDeltaTime)
    {
    }

    virtual void AddHit(const JPH::BodyID& inBodyID) override
    {
      JPH::BodyLockWrite lock(m_system.GetBodyLockInterface(), inBodyID);
      JPH::Body& body = lock.GetBody();
      if (body.IsActive() && body.IsDynamic())
        body.ApplyBuoyancyImpulse(mSurfacePosition, mSurfaceNormal, 1.1f, 0.3f, 0.05f, JPH::Vec3::sZero(), m_system.GetGravity(), mDeltaTime);
    }

  private:
    JPH::PhysicsSystem& m_system;
    JPH::RVec3 mSurfacePosition;
    JPH::Vec3 mSurfaceNormal;
    float mDeltaTime;
  };

  const ezVec3 halfExtents = m_vExtents * 0.5f;
  const ezVec3 globalPos = GetOwner()->GetGlobalPosition();

  JPH::Vec3 joltHalfExtents = ezJoltConversionUtils::ToVec3(halfExtents);
  JPH::AABox waterBox(-joltHalfExtents, joltHalfExtents);
  waterBox.Translate(ezJoltConversionUtils::ToVec3(GetOwner()->GetGlobalPosition()));

  MyCollector collector(joltSystem, globalPos + ezVec3(0, 0, halfExtents.z), ezVec3::MakeAxisZ(), deltaTime.AsFloatInSeconds());
  ezJoltBroadPhaseLayerFilter broadphaseFilter(ezPhysicsShapeType::Dynamic);
  // TODO: collision layer

  joltSystem.GetBroadPhaseQuery().CollideAABox(waterBox, collector, broadphaseFilter);
}

EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltWaterVolumeComponent);

#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Foundation/IO/MemoryStream.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Actors/JoltHeightfieldColliderComponent.h>
#include <JoltPlugin/Resources/JoltHeightfieldResource.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <JoltPlugin/Utilities/JoltStreamUtils.h>
#include <JoltPlugin/Utilities/JoltUserData.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltHeightfieldColliderComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("Heightfield", m_hHeightfield),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Actors"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezJoltHeightfieldColliderComponent::ezJoltHeightfieldColliderComponent() = default;
ezJoltHeightfieldColliderComponent::~ezJoltHeightfieldColliderComponent() = default;

void ezJoltHeightfieldColliderComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_hHeightfield;
}

void ezJoltHeightfieldColliderComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  if (uiVersion >= 2)
  {
    s >> m_hHeightfield;
  }
}

void ezJoltHeightfieldColliderComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (!m_hHeightfield.IsValid())
    return;

  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
  if (pModule == nullptr)
    return;

  ezResourceLock<ezJoltHeightfieldResource> pRes(m_hHeightfield, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pRes.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    ezLog::Warning("ezJoltHeightfieldColliderComponent: could not load heightfield resource '{}'.", m_hHeightfield.GetResourceIdOrDescription());
    return;
  }

  const ezDataBuffer& shapeData = pRes->GetShapeData();
  ezRawMemoryStreamReader memReader(shapeData);
  ezJoltStreamIn jStream(&memReader);
  auto shapeResult = JPH::Shape::sRestoreFromBinaryState(jStream);
  if (shapeResult.HasError())
  {
    ezLog::Error("ezJoltHeightfieldColliderComponent: failed to restore Jolt shape from '{}': {}", m_hHeightfield.GetResourceIdOrDescription(), shapeResult.GetError().c_str());
    return;
  }

  {
    const auto& surfaces = pRes->GetSurfaces();
    ezTempHybridArray<JPH::PhysicsMaterialRefC, 8> materials;
    materials.SetCount(surfaces.GetCount());
    for (ezUInt32 i = 0; i < surfaces.GetCount(); ++i)
    {
      const ezJoltMaterial* pResolved = nullptr;
      if (surfaces[i].IsValid())
      {
        ezResourceLock<ezSurfaceResource> pSurf(surfaces[i], ezResourceAcquireMode::BlockTillLoaded_NeverFail);
        if (pSurf.GetAcquireResult() == ezResourceAcquireResult::Final && pSurf->m_pPhysicsMaterialJolt != nullptr)
          pResolved = reinterpret_cast<const ezJoltMaterial*>(pSurf->m_pPhysicsMaterialJolt);
      }
      materials[i] = pResolved != nullptr ? pResolved : ezJoltCore::GetDefaultMaterial();
    }
    if (!materials.IsEmpty())
      shapeResult.Get()->RestoreMaterialState(materials.GetData(), materials.GetCount());
  }

  // Wrap in a RotatedTranslatedShape to apply the +90° X rotation that maps Jolt's Y-up to ezEngine's Z-up.
  const JPH::Quat qRotX90 = JPH::Quat::sRotation(JPH::Vec3::sAxisX(), 0.5f * JPH::JPH_PI);
  JPH::RotatedTranslatedShapeSettings rtsSettings(JPH::Vec3::sZero(), qRotX90, shapeResult.Get());
  JPH::ShapeSettings::ShapeResult rtsResult = rtsSettings.Create();
  if (rtsResult.HasError())
  {
    ezLog::Error("ezJoltHeightfieldColliderComponent: failed to wrap shape: {}", rtsResult.GetError().c_str());
    return;
  }

  ezJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);
  m_uiObjectFilterID = pModule->CreateObjectFilterID();

  // Use the first resolved material for body-level friction/restitution, or the default.
  const ezJoltMaterial* pFirstMaterial = ezJoltCore::GetDefaultMaterial();
  if (!pRes->GetSurfaces().IsEmpty() && pRes->GetSurfaces()[0].IsValid())
  {
    ezResourceLock<ezSurfaceResource> pSurf(pRes->GetSurfaces()[0], ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pSurf.GetAcquireResult() == ezResourceAcquireResult::Final && pSurf->m_pPhysicsMaterialJolt != nullptr)
      pFirstMaterial = reinterpret_cast<const ezJoltMaterial*>(pSurf->m_pPhysicsMaterialJolt);
  }

  const ezSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  JPH::BodyCreationSettings bodyCfg;
  bodyCfg.SetShape(rtsResult.Get());
  bodyCfg.mPosition = ezJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = ezJoltConversionUtils::ToQuat(trans.m_Rotation).Normalized();
  bodyCfg.mMotionType = JPH::EMotionType::Static;
  bodyCfg.mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(pRes->GetCollisionLayer(), ezJoltBroadphaseLayer::Static);
  bodyCfg.mRestitution = pFirstMaterial->m_fRestitution;
  bodyCfg.mFriction = pFirstMaterial->m_fFriction;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
  bodyCfg.mEnhancedInternalEdgeRemoval = true;
  bodyCfg.mUserData = reinterpret_cast<ezUInt64>(pUserData);

  auto* pBodies = &pModule->GetJoltSystem()->GetBodyInterface();
  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  if (pBody == nullptr)
  {
    ezLog::Error("ezJoltHeightfieldColliderComponent: Jolt body creation failed. Increase the maximum number of bodies.");
    pModule->DeallocateUserData(m_uiUserDataIndex);
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
    m_uiUserDataIndex = ezInvalidIndex;
    m_uiObjectFilterID = ezInvalidIndex;
    return;
  }

  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();
  pModule->QueueBodyToAdd(pBody, false);
}

void ezJoltHeightfieldColliderComponent::OnDeactivated()
{
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  if (pModule != nullptr)
  {
    JPH::BodyID bodyId(m_uiJoltBodyID);

    if (!bodyId.IsInvalid())
    {
      auto* pSystem = pModule->GetJoltSystem();
      auto* pBodies = &pSystem->GetBodyInterface();

      if (pBodies->IsAdded(bodyId))
      {
        pBodies->RemoveBody(bodyId);
      }
      else
      {
        pModule->RemoveBodyFromQueue(bodyId);
      }

      pBodies->DestroyBody(bodyId);
      m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
    }

    pModule->DeallocateUserData(m_uiUserDataIndex);
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
  }

  SUPER::OnDeactivated();
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltHeightfieldColliderComponent);

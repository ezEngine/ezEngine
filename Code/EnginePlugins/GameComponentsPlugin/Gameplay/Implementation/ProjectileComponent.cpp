#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameComponentsPlugin/Gameplay/ProjectileComponent.h>
#include <GameEngine/Messages/DamageMessage.h>
#include <GameEngine/Physics/ImpulseType.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezProjectileReaction, 2)
  EZ_ENUM_CONSTANT(ezProjectileReaction::Absorb),
  EZ_ENUM_CONSTANT(ezProjectileReaction::Reflect),
  EZ_ENUM_CONSTANT(ezProjectileReaction::Bounce),
  EZ_ENUM_CONSTANT(ezProjectileReaction::Attach),
  EZ_ENUM_CONSTANT(ezProjectileReaction::PassThrough)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezProjectileBounceOrientation, 1)
  EZ_ENUM_CONSTANT(ezProjectileBounceOrientation::Reflection),
  EZ_ENUM_CONSTANT(ezProjectileBounceOrientation::Spinning)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezProjectileSurfaceInteraction, ezNoBase, 3, ezRTTIDefaultAllocator<ezProjectileSurfaceInteraction>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("Surface", m_hSurface)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),
    EZ_ENUM_MEMBER_PROPERTY("Reaction", ezProjectileReaction, m_Reaction),
    EZ_MEMBER_PROPERTY("Interaction", m_sInteraction)->AddAttributes(new ezDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    EZ_MEMBER_PROPERTY("ImpulseType", m_uiImpulseType)->AddAttributes(new ezDynamicEnumAttribute("PhysicsImpulseType")),
    EZ_MEMBER_PROPERTY("Impulse", m_fImpulse),
    EZ_MEMBER_PROPERTY("Damage", m_fDamage),
    EZ_MEMBER_PROPERTY("InertiaRatio", m_fInertiaRatio)->AddAttributes(new ezDefaultValueAttribute(5.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezProjectileComponent, 8, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Speed", m_fMetersPerSecond)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("GravityMultiplier", m_fGravityMultiplier),
    EZ_MEMBER_PROPERTY("MaxLifetime", m_MaxLifetime)->AddAttributes(new ezClampValueAttribute(ezTime(), ezVariant())),
    EZ_MEMBER_PROPERTY("SpawnPrefabOnStatic", m_bSpawnPrefabOnStatic),
    EZ_RESOURCE_MEMBER_PROPERTY("OnDeathPrefab", m_hDeathPrefab)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ENUM_MEMBER_PROPERTY("BounceOrientation", ezProjectileBounceOrientation, m_BounceOrientation)->AddAttributes(new ezDefaultValueAttribute((ezInt8)ezProjectileBounceOrientation::Reflection)),
    EZ_MEMBER_PROPERTY("StaticVelocityRatio", m_fStaticVelocityRatio)->AddAttributes(new ezDefaultValueAttribute(0.05f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_BITFLAGS_MEMBER_PROPERTY("ShapeTypesToHit", ezPhysicsShapeType, m_ShapeTypesToHit)->AddAttributes(new ezDefaultValueAttribute(ezVariant(ezPhysicsShapeType::Default & ~(ezPhysicsShapeType::Trigger)))),
    EZ_ACCESSOR_PROPERTY("FallbackSurface", GetFallbackSurfaceFile, SetFallbackSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),
    EZ_ARRAY_MEMBER_PROPERTY("Interactions", m_SurfaceInteractions),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgComponentInternalTrigger, OnTriggered),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.4f, ezColor::OrangeRed),
    new ezSphereManipulatorAttribute("Radius"),
    new ezSphereVisualizerAttribute("Radius"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// \brief Helper function to recalculate the sphere position from hit position
  /// \param vOrigin is expected to be the initial position of the particle before SweepTest
  ezVec3 CalculateSphereCenterPosition(const ezVec3& vOrigin, const ezVec3& vDirection, const ezPhysicsCastResult& castResult, float fPenetrationDepth)
  {
    if (castResult.m_fDistance == 0.0f)
    {
      // It says that the hit position is "behind" (if we move along vDirection) vOrigin
      // Thus, return the original position
      return vOrigin;
    }

    return vOrigin + vDirection * (castResult.m_fDistance + fPenetrationDepth);
  }
} // namespace

ezProjectileComponent::ezProjectileComponent()
{
  m_fMetersPerSecond = 10.0f;
  m_uiCollisionLayer = 0;
  m_fRadius = 0.0f;
  m_BounceOrientation = ezProjectileBounceOrientation::Reflection;
  m_fStaticVelocityRatio = 0.05f;
  m_fGravityMultiplier = 0.0f;
  m_vVelocity.SetZero();
  m_bSpawnPrefabOnStatic = false;
}

ezProjectileComponent::~ezProjectileComponent() = default;

void ezProjectileComponent::Update()
{
  const float fPenetrationDepth = m_fRadius * 0.5f;

  if (m_fGravityMultiplier == 0.0f && m_fMetersPerSecond == 0.0f)
    return;

  ezPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetModule<ezPhysicsWorldModuleInterface>();

  if (pPhysicsInterface)
  {
    ezGameObject* pEntity = GetOwner();
    const ezVec3 vCurPosition = pEntity->GetGlobalPosition();

    const float fTimeDiff = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();

    ezVec3 vNewPosition;

    // gravity
    if (m_fGravityMultiplier != 0.0f && m_fMetersPerSecond > 0.0f) // mps == 0 for attached state
    {
      const ezVec3 vGravity = pPhysicsInterface->GetGravity() * m_fGravityMultiplier;

      m_vVelocity += vGravity * fTimeDiff;
    }

    ezVec3 vCurDirection = m_vVelocity * fTimeDiff;
    float fDistance = 0.0f;

    if (!vCurDirection.IsZero())
      fDistance = vCurDirection.GetLengthAndNormalize();

    ezPhysicsQueryParameters queryParams(m_uiCollisionLayer);
    queryParams.m_bIgnoreInitialOverlap = true;
    queryParams.m_ShapeTypes = m_ShapeTypesToHit;

    ezPhysicsCastResult castResult;
    if (QueryCollision(*pPhysicsInterface, castResult, vCurPosition, vCurDirection, fDistance, queryParams))
    {
      const ezVec3 vNewCenterPosition = (m_fRadius > 0.0f)
        ? CalculateSphereCenterPosition(vCurPosition, vCurDirection, castResult, fPenetrationDepth)
        : castResult.m_vPosition;

      const ezSurfaceResourceHandle hSurface = castResult.m_hSurface.IsValid() ? castResult.m_hSurface : m_hFallbackSurface;

      const ezInt32 iInteraction = FindSurfaceInteraction(hSurface);

      if (iInteraction == -1)
      {
        GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
        vNewPosition = vNewCenterPosition;
      }
      else
      {
        const auto& interaction = m_SurfaceInteractions[iInteraction];

        if (!interaction.m_sInteraction.IsEmpty())
        {
          TriggerSurfaceInteraction(hSurface, castResult.m_hActorObject, castResult.m_vPosition, castResult.m_vNormal, vCurDirection, interaction.m_sInteraction);
        }

        // if we hit some valid object
        if (!castResult.m_hActorObject.IsInvalidated())
        {
          ezGameObject* pObject = nullptr;

          // apply a physical impulse
          if (interaction.m_uiImpulseType >= ezImpulseTypeConfig::FirstValidKey || (interaction.m_uiImpulseType == ezImpulseTypeConfig::CustomValueKey && interaction.m_fImpulse > 0.0f))
          {
            if (GetWorld()->TryGetObject(castResult.m_hActorObject, pObject))
            {
              ezMsgPhysicsAddImpulse msg;
              msg.m_uiImpulseType = interaction.m_uiImpulseType;
              msg.m_vGlobalPosition = castResult.m_vPosition;
              msg.m_vImpulse = vCurDirection;
              msg.m_uiObjectFilterID = castResult.m_uiObjectFilterID;
              msg.m_pInternalPhysicsShape = castResult.m_pInternalPhysicsShape;
              msg.m_pInternalPhysicsActor = castResult.m_pInternalPhysicsActor;

              if (interaction.m_uiImpulseType == ezImpulseTypeConfig::CustomValueKey)
              {
                msg.m_vImpulse *= interaction.m_fImpulse;
              }

              pObject->SendMessage(msg);
            }
          }

          // apply damage
          if (interaction.m_fDamage > 0.0f)
          {
            // skip the TryGetObject if we already did that above
            if (pObject != nullptr || GetWorld()->TryGetObject(castResult.m_hShapeObject, pObject))
            {
              ezMsgDamage msg;
              msg.m_fDamage = interaction.m_fDamage;
              msg.m_vGlobalPosition = castResult.m_vPosition;
              msg.m_vImpactDirection = vCurDirection;

              ezGameObject* pHitShape = nullptr;
              if (GetWorld()->TryGetObject(castResult.m_hShapeObject, pHitShape))
              {
                msg.m_sHitObjectName = pHitShape->GetName();
              }
              else
              {
                msg.m_sHitObjectName = pObject->GetName();
              }

              pObject->SendEventMessage(msg, this);
            }
          }
        }

        if (interaction.m_Reaction == ezProjectileReaction::Absorb)
        {
          SpawnDeathPrefab();


          GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
          vNewPosition = vNewCenterPosition;
        }
        else if (interaction.m_Reaction == ezProjectileReaction::Reflect || interaction.m_Reaction == ezProjectileReaction::Bounce)
        {
          vNewPosition = vCurPosition;
          const float velocityToNormalProj = m_vVelocity.Dot(castResult.m_vNormal);
          if (velocityToNormalProj < 0.0f)
          {
            // the same as m_vVelocity.GetReflectedVector but reuse precalculated velocityToNormalProj
            ezVec3 vNewVelocity = m_vVelocity - 2.0f * velocityToNormalProj * castResult.m_vNormal;

            // Position of the projectile at the moment of reflection

            if (interaction.m_Reaction == ezProjectileReaction::Bounce)
            {
              ezResourceLock<ezSurfaceResource> pSurface(hSurface, ezResourceAcquireMode::BlockTillLoaded);

              if (pSurface)
              {
                vNewVelocity *= pSurface->GetDescriptor().m_fPhysicsRestitution;
              }

              if (ShouldStopProjectile(*pPhysicsInterface, castResult, vNewVelocity))
              {
                vNewVelocity = ezVec3::MakeZero();
                m_fGravityMultiplier = 0.0f;

                if (m_bSpawnPrefabOnStatic)
                {
                  SpawnDeathPrefab();
                  GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
                }
              }
            }

            const ezVec3 vPositionOnReflection = vCurPosition + castResult.m_fDistance * vCurDirection;
            if (m_BounceOrientation == ezProjectileBounceOrientation::Reflection)
            {
              ApplyReflectionRotation(vCurDirection, castResult.m_vNormal);
            }
            else
            {
              ApplySpinningRotation(interaction, castResult, vPositionOnReflection, vCurDirection, vNewVelocity);
            }
            const float fAbsVelocity = m_vVelocity.GetLength();
            // condition velocityToNormalProj < 0.0f implies fAbsVelocity is non-zero
            const float fTimeBeforeReflection = castResult.m_fDistance / fAbsVelocity;
            const float fTimeLeft = ezMath::Max(0.0f, fTimeDiff - fTimeBeforeReflection);
            // Path travelled back after the reflection
            vNewPosition = vPositionOnReflection + vNewVelocity * fTimeLeft;
            m_vVelocity = vNewVelocity;
          }
          else
          {
            vNewPosition += m_vVelocity * fTimeDiff;
          }
        }
        else if (interaction.m_Reaction == ezProjectileReaction::Attach)
        {
          m_fMetersPerSecond = 0.0f;
          m_fGravityMultiplier = 0.0f;
          vNewPosition = vNewCenterPosition;

          ezGameObject* pObject;
          if (GetWorld()->TryGetObject(castResult.m_hActorObject, pObject))
          {
            pObject->AddChild(GetOwner()->GetHandle(), ezTransformPreservation::Enum::PreserveGlobal);
          }
        }
        else if (interaction.m_Reaction == ezProjectileReaction::PassThrough)
        {
          vNewPosition = vCurPosition + fDistance * vCurDirection;
        }
      }
    }
    else
    {
      vNewPosition = vCurPosition + fDistance * vCurDirection;
    }

    GetOwner()->SetGlobalPosition(vNewPosition);
  }
}

void ezProjectileComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fMetersPerSecond;
  s << m_fGravityMultiplier;
  s << m_uiCollisionLayer;
  s << m_MaxLifetime;
  s << m_hDeathPrefab;

  // Version 3
  s << m_hFallbackSurface;

  s << m_SurfaceInteractions.GetCount();
  for (const auto& ia : m_SurfaceInteractions)
  {
    s << ia.m_hSurface;

    ezProjectileReaction::StorageType storage = ia.m_Reaction;
    s << storage;

    s << ia.m_sInteraction;

    // Version 3
    s << ia.m_fImpulse;

    // Version 4
    s << ia.m_fDamage;

    // Version 7
    s << ia.m_uiImpulseType;
  }

  // Version 5
  s << m_ShapeTypesToHit;

  // Version 6
  s << m_bSpawnPrefabOnStatic;

  // Version 8
  s << m_fRadius;
  ezProjectileBounceOrientation::StorageType bounceOrientation = m_BounceOrientation;
  s << bounceOrientation;
  s << m_fStaticVelocityRatio;

  // Version 8
  for (const auto& ia : m_SurfaceInteractions)
  {
    s << ia.m_fInertiaRatio;
  }
}

void ezProjectileComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fMetersPerSecond;
  s >> m_fGravityMultiplier;
  s >> m_uiCollisionLayer;
  s >> m_MaxLifetime;
  s >> m_hDeathPrefab;

  if (uiVersion >= 3)
  {
    s >> m_hFallbackSurface;
  }

  ezUInt32 count;
  s >> count;
  m_SurfaceInteractions.SetCount(count);
  for (ezUInt32 i = 0; i < count; ++i)
  {
    auto& ia = m_SurfaceInteractions[i];
    s >> ia.m_hSurface;

    ezProjectileReaction::StorageType storage = 0;
    s >> storage;
    ia.m_Reaction = (ezProjectileReaction::Enum)storage;

    s >> ia.m_sInteraction;

    if (uiVersion >= 3)
    {
      s >> ia.m_fImpulse;
    }

    if (uiVersion >= 4)
    {
      s >> ia.m_fDamage;
    }

    if (uiVersion >= 7)
    {
      s >> ia.m_uiImpulseType;
    }
  }

  if (uiVersion >= 5)
  {
    s >> m_ShapeTypesToHit;
  }

  if (uiVersion >= 6)
  {
    s >> m_bSpawnPrefabOnStatic;
  }

  if (uiVersion >= 8)
  {
    s >> m_fRadius;
    ezProjectileBounceOrientation::StorageType bounceOrientation = ezProjectileBounceOrientation::Reflection;
    s >> bounceOrientation;
    m_BounceOrientation = (ezProjectileBounceOrientation::Enum)bounceOrientation;
    s >> m_fStaticVelocityRatio;

    for (auto& ia : m_SurfaceInteractions)
    {
      s >> ia.m_fInertiaRatio;
    }
  }
  else
  {
    m_fRadius = 0.0f;
    m_BounceOrientation = ezProjectileBounceOrientation::Reflection;
    m_fStaticVelocityRatio = 0.05f;
  }
}


bool ezProjectileComponent::QueryCollision(const ezPhysicsWorldModuleInterface& physicsInterface, ezPhysicsCastResult& out_result, const ezVec3& vStart, const ezVec3& vDirection, float fDistance, const ezPhysicsQueryParameters& queryParams) const
{
  if (m_fRadius > 0.0f)
  {
    return physicsInterface.SweepTestSphere(out_result, m_fRadius, vStart, vDirection, fDistance, queryParams);
  }

  return physicsInterface.Raycast(out_result, vStart, vDirection, fDistance, queryParams);
}

void ezProjectileComponent::ApplyReflectionRotation(const ezVec3& vCurDirection, const ezVec3& vSurfaceNormal)
{
  const ezVec3 vNewDirection = vCurDirection.GetReflectedVector(vSurfaceNormal);
  const ezQuat qRot = ezQuat::MakeShortestRotation(vCurDirection, vNewDirection);
  GetOwner()->SetGlobalRotation(qRot * GetOwner()->GetGlobalRotation());
}

void ezProjectileComponent::ApplySpinningRotation(const ezProjectileSurfaceInteraction& interaction,
                                                  const ezPhysicsCastResult& castResult,
                                                  const ezVec3& vPositionOnReflection,
                                                  const ezVec3& vCurDirection,
                                                  const ezVec3& vNewVelocity)
{
  if (interaction.m_fInertiaRatio == 0.0f)
    return;

  const ezVec3 vRelativeHitPosOnReflection = castResult.m_vPosition - vPositionOnReflection;
  const float vRelativeHitPosLength = vRelativeHitPosOnReflection.GetLength();
  ezVec3 vForceRadialVector;
  if (m_fRadius > 0.0f && vRelativeHitPosLength > 0.0f)
  {
    // if the projectile has a radius it is natural to assume that the torque is created
    // by the force acting on the radial vector connecting the center to the contact point
    vForceRadialVector = vRelativeHitPosOnReflection / vRelativeHitPosLength;
  }
  else
  {
    // Otherwise assume radial vector coincides with direction
    vForceRadialVector = vCurDirection;
  }

  // Assuming the timeDiff is small we can say that the difference between velocities
  // is proportional to the acting force
  ezVec3 vEffectiveForce = m_vVelocity - vNewVelocity;
  // We assume that this force is responsible for torque and angular momentum
  ezVec3 vEffectiveTorque = -vForceRadialVector.CrossRH(vEffectiveForce) / interaction.m_fInertiaRatio;

  const float fAngle = ezMath::Min(vEffectiveTorque.GetLength(), ezMath::Pi<float>());
  if (fAngle > 0.0f)
  {
    ezAngle angle = ezAngle::MakeFromRadian(fAngle);
    ezQuat qRot = ezQuat::MakeFromAxisAndAngle(vEffectiveTorque.GetNormalized(), angle);
    GetOwner()->SetGlobalRotation(qRot * GetOwner()->GetGlobalRotation());
  }
}

bool ezProjectileComponent::ShouldStopProjectile(const ezPhysicsWorldModuleInterface& physicsInterface, const ezPhysicsCastResult& castResult, const ezVec3& vVelocity)
{
  const ezVec3 vGravity = physicsInterface.GetGravity();
  if (!vGravity.IsZero())
  {
    const ezVec3 vGravityDir = vGravity.GetNormalized();
    // Check that projectile has hit the ground
    // if not - return false to make sure it won't hang in the air after being stopped
    if (-vGravityDir.Dot(castResult.m_vNormal) < ezMath::Cos(ezAngle::MakeFromDegree(40.f)))
    {
      return false;
    }
  }

  return vVelocity.GetLength() < m_fStaticVelocityRatio * m_fMetersPerSecond;
}

ezInt32 ezProjectileComponent::FindSurfaceInteraction(const ezSurfaceResourceHandle& hSurface) const
{
  ezSurfaceResourceHandle hCurSurf = hSurface;

  while (hCurSurf.IsValid())
  {
    for (ezUInt32 i = 0; i < m_SurfaceInteractions.GetCount(); ++i)
    {
      if (hCurSurf == m_SurfaceInteractions[i].m_hSurface)
        return i;
    }

    // get parent surface
    {
      ezResourceLock<ezSurfaceResource> pSurf(hCurSurf, ezResourceAcquireMode::BlockTillLoaded);
      hCurSurf = pSurf->GetDescriptor().m_hBaseSurface;
    }
  }

  return -1;
}


void ezProjectileComponent::TriggerSurfaceInteraction(const ezSurfaceResourceHandle& hSurface, ezGameObjectHandle hObject, const ezVec3& vPos, const ezVec3& vNormal, const ezVec3& vDirection, const char* szInteraction)
{
  ezResourceLock<ezSurfaceResource> pSurface(hSurface, ezResourceAcquireMode::BlockTillLoaded);
  pSurface->InteractWithSurface(GetWorld(), hObject, vPos, vNormal, vDirection, ezTempHashedString(szInteraction), &GetOwner()->GetTeamID());
}

static ezHashedString s_sSuicide = ezMakeHashedString("Suicide");

void ezProjectileComponent::OnSimulationStarted()
{
  if (m_MaxLifetime.GetSeconds() > 0.0)
  {
    ezMsgComponentInternalTrigger msg;
    msg.m_sMessage = s_sSuicide;

    PostMessage(msg, m_MaxLifetime);

    // make sure the prefab is available when the projectile dies
    if (m_hDeathPrefab.IsValid())
    {
      ezResourceManager::PreloadResource(m_hDeathPrefab);
    }
  }

  m_vVelocity = GetOwner()->GetGlobalDirForwards() * m_fMetersPerSecond;
}

void ezProjectileComponent::SpawnDeathPrefab()
{
  if (!m_bSpawnPrefabOnStatic)
  {
    return;
  }

  if (m_hDeathPrefab.IsValid())
  {
    ezResourceLock<ezPrefabResource> pPrefab(m_hDeathPrefab, ezResourceAcquireMode::AllowLoadingFallback);

    ezPrefabInstantiationOptions options;
    options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

    pPrefab->InstantiatePrefab(*GetWorld(), GetOwner()->GetGlobalTransform(), options, nullptr);
  }
}

void ezProjectileComponent::OnTriggered(ezMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != s_sSuicide)
    return;

  SpawnDeathPrefab();

  GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
}

void ezProjectileComponent::SetFallbackSurfaceFile(ezStringView sFile)
{
  if (!sFile.IsEmpty())
  {
    m_hFallbackSurface = ezResourceManager::LoadResource<ezSurfaceResource>(sFile);
  }
  else
  {
    m_hFallbackSurface = {};
  }

  if (m_hFallbackSurface.IsValid())
    ezResourceManager::PreloadResource(m_hFallbackSurface);
}

ezStringView ezProjectileComponent::GetFallbackSurfaceFile() const
{
  if (!m_hFallbackSurface.IsValid())
    return "";

  return m_hFallbackSurface.GetResourceID();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezProjectileComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezProjectileComponentPatch_1_2()
    : ezGraphPatch("ezProjectileComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Gravity Multiplier", "GravityMultiplier");
    pNode->RenameProperty("Max Lifetime", "MaxLifetime");
    pNode->RenameProperty("Timeout Prefab", "TimeoutPrefab");
    pNode->RenameProperty("Collision Layer", "CollisionLayer");
  }
};

class ezProjectileComponentPatch_5_6 : public ezGraphPatch
{
public:
  ezProjectileComponentPatch_5_6()
    : ezGraphPatch("ezProjectileComponent", 6)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("TimeoutPrefab", "DeathPrefab");
  }
};

ezProjectileComponentPatch_1_2 g_ezProjectileComponentPatch_1_2;


EZ_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Gameplay_Implementation_ProjectileComponent);

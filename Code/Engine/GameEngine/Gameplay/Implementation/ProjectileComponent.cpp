#include <GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/ProjectileComponent.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <GameEngine/Messages/DamageMessage.h>
#include <GameEngine/Prefabs/PrefabResource.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezProjectileReaction, 1)
  EZ_ENUM_CONSTANT(ezProjectileReaction::Absorb),
  EZ_ENUM_CONSTANT(ezProjectileReaction::Reflect),
  EZ_ENUM_CONSTANT(ezProjectileReaction::Attach),
  EZ_ENUM_CONSTANT(ezProjectileReaction::PassThrough)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezProjectileSurfaceInteraction, ezNoBase, 3, ezRTTIDefaultAllocator<ezProjectileSurfaceInteraction>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Surface", GetSurface, SetSurface)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
    EZ_ENUM_MEMBER_PROPERTY("Reaction", ezProjectileReaction, m_Reaction),
    EZ_MEMBER_PROPERTY("Interaction", m_sInteraction),
    EZ_MEMBER_PROPERTY("Impulse", m_fImpulse),
    EZ_MEMBER_PROPERTY("Damage", m_fDamage),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezProjectileComponent, 4, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Speed", m_fMetersPerSecond)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("GravityMultiplier", m_fGravityMultiplier),
    EZ_MEMBER_PROPERTY("MaxLifetime", m_MaxLifetime)->AddAttributes(new ezClampValueAttribute(ezTime(), ezVariant())),
    EZ_ACCESSOR_PROPERTY("OnTimeoutSpawn", GetTimeoutPrefab, SetTimeoutPrefab)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_ACCESSOR_PROPERTY("FallbackSurface", GetFallbackSurfaceFile, SetFallbackSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
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
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.2f, ezColor::OrangeRed),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezProjectileSurfaceInteraction::SetSurface(const char* szSurface)
{
  ezSurfaceResourceHandle hSurface;

  if (!ezStringUtils::IsNullOrEmpty(szSurface))
  {
    hSurface = ezResourceManager::LoadResource<ezSurfaceResource>(szSurface);
  }

  m_hSurface = hSurface;
}

const char* ezProjectileSurfaceInteraction::GetSurface() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}

ezProjectileComponent::ezProjectileComponent()
{
  m_fMetersPerSecond = 10.0f;
  m_uiCollisionLayer = 0;
  m_fGravityMultiplier = 0.0f;
  m_vVelocity.SetZero();
}

void ezProjectileComponent::Update()
{
  ezPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetModule<ezPhysicsWorldModuleInterface>();

  if (pPhysicsInterface)
  {
    ezGameObject* pEntity = GetOwner();

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

    ezPhysicsHitResult hitResult;
    if (pPhysicsInterface->CastRay(pEntity->GetGlobalPosition(), vCurDirection, fDistance, m_uiCollisionLayer, hitResult))
    {
      const ezSurfaceResourceHandle hSurface = hitResult.m_hSurface.IsValid() ? hitResult.m_hSurface : m_hFallbackSurface;

      const ezInt32 iInteraction = FindSurfaceInteraction(hSurface);

      if (iInteraction == -1)
      {
        GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
        vNewPosition = hitResult.m_vPosition;
      }
      else
      {
        const auto& interaction = m_SurfaceInteractions[iInteraction];

        if (!interaction.m_sInteraction.IsEmpty())
        {
          TriggerSurfaceInteraction(
            hSurface, hitResult.m_hActorObject, hitResult.m_vPosition, hitResult.m_vNormal, vCurDirection, interaction.m_sInteraction);
        }

        // if we hit some valid object
        if (!hitResult.m_hActorObject.IsInvalidated())
        {
          ezGameObject* pObject = nullptr;

          // apply a physical impulse
          if (interaction.m_fImpulse > 0.0f)
          {
            if (GetWorld()->TryGetObject(hitResult.m_hActorObject, pObject))
            {
              ezMsgPhysicsAddImpulse msg;
              msg.m_vGlobalPosition = hitResult.m_vPosition;
              msg.m_vImpulse = vCurDirection * interaction.m_fImpulse;
              msg.m_uiShapeId = hitResult.m_uiShapeId;

              pObject->SendMessage(msg);
            }
          }

          // apply damage
          if (interaction.m_fDamage > 0.0f)
          {
            // skip the TryGetObject if we already did that above
            if (pObject != nullptr || GetWorld()->TryGetObject(hitResult.m_hActorObject, pObject))
            {
              ezMsgDamage msg;
              msg.m_fDamage = interaction.m_fDamage;

              pObject->SendMessage(msg);
            }
          }
        }

        if (interaction.m_Reaction == ezProjectileReaction::Absorb)
        {
          GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
          vNewPosition = hitResult.m_vPosition;
        }
        else if (interaction.m_Reaction == ezProjectileReaction::Reflect)
        {
          /// \todo Should reflect around the actual hit position
          /// \todo Should preserve travel distance while reflecting

          // const float fLength = (vPos - pEntity->GetGlobalPosition()).GetLength();

          vNewPosition = pEntity->GetGlobalPosition(); // vPos;

          const ezVec3 vNewDirection = vCurDirection.GetReflectedVector(hitResult.m_vNormal);

          ezQuat qRot;
          qRot.SetShortestRotation(vCurDirection, vNewDirection);

          GetOwner()->SetGlobalRotation(qRot * GetOwner()->GetGlobalRotation());

          m_vVelocity = qRot * m_vVelocity;
        }
        else if (interaction.m_Reaction == ezProjectileReaction::Attach)
        {
          m_fMetersPerSecond = 0.0f;
          vNewPosition = hitResult.m_vPosition;

          ezGameObject* pObject;
          if (GetWorld()->TryGetObject(hitResult.m_hActorObject, pObject))
          {
            pObject->AddChild(GetOwner()->GetHandle(), ezGameObject::TransformPreservation::PreserveGlobal);
          }
        }
        else if (interaction.m_Reaction == ezProjectileReaction::PassThrough)
        {
          vNewPosition = pEntity->GetGlobalPosition() + fDistance * vCurDirection;
        }
      }
    }
    else
    {
      vNewPosition = pEntity->GetGlobalPosition() + fDistance * vCurDirection;
    }

    GetOwner()->SetGlobalPosition(vNewPosition);
  }
}

void ezProjectileComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fMetersPerSecond;
  s << m_fGravityMultiplier;
  s << m_uiCollisionLayer;
  s << m_MaxLifetime;
  s << m_hTimeoutPrefab;

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
  }
}

void ezProjectileComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fMetersPerSecond;
  s >> m_fGravityMultiplier;
  s >> m_uiCollisionLayer;
  s >> m_MaxLifetime;
  s >> m_hTimeoutPrefab;

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
  }
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


void ezProjectileComponent::TriggerSurfaceInteraction(const ezSurfaceResourceHandle& hSurface, ezGameObjectHandle hObject,
  const ezVec3& vPos, const ezVec3& vNormal, const ezVec3& vDirection, const char* szInteraction)
{
  ezResourceLock<ezSurfaceResource> pSurface(hSurface, ezResourceAcquireMode::BlockTillLoaded);
  pSurface->InteractWithSurface(
    GetWorld(), hObject, vPos, vNormal, vDirection, ezTempHashedString(szInteraction), &GetOwner()->GetTeamID());
}


void ezProjectileComponent::OnSimulationStarted()
{
  if (m_MaxLifetime.GetSeconds() > 0.0)
  {
    ezMsgComponentInternalTrigger msg;
    msg.m_uiUsageStringHash = ezTempHashedString::ComputeHash("Suicide");

    PostMessage(msg, ezObjectMsgQueueType::NextFrame, m_MaxLifetime);

    // make sure the prefab is available when the projectile dies
    if (m_hTimeoutPrefab.IsValid())
    {
      ezResourceManager::PreloadResource(m_hTimeoutPrefab);
    }
  }

  m_vVelocity = GetOwner()->GetGlobalDirForwards() * m_fMetersPerSecond;
}

void ezProjectileComponent::OnTriggered(ezMsgComponentInternalTrigger& msg)
{
  if (msg.m_uiUsageStringHash != ezTempHashedString::ComputeHash("Suicide"))
    return;

  if (m_hTimeoutPrefab.IsValid())
  {
    ezResourceLock<ezPrefabResource> pPrefab(m_hTimeoutPrefab, ezResourceAcquireMode::AllowLoadingFallback);

    pPrefab->InstantiatePrefab(
      *GetWorld(), GetOwner()->GetGlobalTransform(), ezGameObjectHandle(), nullptr, &GetOwner()->GetTeamID(), nullptr, false);
  }

  GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
}


void ezProjectileComponent::SetTimeoutPrefab(const char* szPrefab)
{
  ezPrefabResourceHandle hPrefab;

  if (!ezStringUtils::IsNullOrEmpty(szPrefab))
  {
    hPrefab = ezResourceManager::LoadResource<ezPrefabResource>(szPrefab);
  }

  m_hTimeoutPrefab = hPrefab;
}

const char* ezProjectileComponent::GetTimeoutPrefab() const
{
  if (!m_hTimeoutPrefab.IsValid())
    return "";

  return m_hTimeoutPrefab.GetResourceID();
}

void ezProjectileComponent::SetFallbackSurfaceFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hFallbackSurface = ezResourceManager::LoadResource<ezSurfaceResource>(szFile);
  }
  if (m_hFallbackSurface.IsValid())
    ezResourceManager::PreloadResource(m_hFallbackSurface);
}

const char* ezProjectileComponent::GetFallbackSurfaceFile() const
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

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Gravity Multiplier", "GravityMultiplier");
    pNode->RenameProperty("Max Lifetime", "MaxLifetime");
    pNode->RenameProperty("Timeout Prefab", "TimeoutPrefab");
    pNode->RenameProperty("Collision Layer", "CollisionLayer");
  }
};

ezProjectileComponentPatch_1_2 g_ezProjectileComponentPatch_1_2;



EZ_STATICLINK_FILE(GameEngine, GameEngine_Components_Implementation_ProjectileComponent);

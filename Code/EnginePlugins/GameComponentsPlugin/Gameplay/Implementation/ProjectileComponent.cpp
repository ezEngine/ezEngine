#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameComponentsPlugin/Gameplay/ProjectileComponent.h>
#include <GameEngine/Messages/DamageMessage.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezProjectileReaction, 2)
  EZ_ENUM_CONSTANT(ezProjectileReaction::Absorb),
  EZ_ENUM_CONSTANT(ezProjectileReaction::Reflect),
  EZ_ENUM_CONSTANT(ezProjectileReaction::Bounce),
  EZ_ENUM_CONSTANT(ezProjectileReaction::Attach),
  EZ_ENUM_CONSTANT(ezProjectileReaction::PassThrough)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezProjectileSurfaceInteraction, ezNoBase, 3, ezRTTIDefaultAllocator<ezProjectileSurfaceInteraction>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("Surface", m_hSurface)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),
    EZ_ENUM_MEMBER_PROPERTY("Reaction", ezProjectileReaction, m_Reaction),
    EZ_MEMBER_PROPERTY("Interaction", m_sInteraction)->AddAttributes(new ezDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    EZ_MEMBER_PROPERTY("Impulse", m_fImpulse),
    EZ_MEMBER_PROPERTY("Damage", m_fDamage),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezProjectileComponent, 6, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Speed", m_fMetersPerSecond)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("GravityMultiplier", m_fGravityMultiplier),
    EZ_MEMBER_PROPERTY("MaxLifetime", m_MaxLifetime)->AddAttributes(new ezClampValueAttribute(ezTime(), ezVariant())),
    EZ_MEMBER_PROPERTY("SpawnPrefabOnStatic", m_bSpawnPrefabOnStatic),
    EZ_RESOURCE_MEMBER_PROPERTY("OnDeathPrefab", m_hDeathPrefab)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
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
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezProjectileComponent::ezProjectileComponent()
{
  m_fMetersPerSecond = 10.0f;
  m_uiCollisionLayer = 0;
  m_fGravityMultiplier = 0.0f;
  m_vVelocity.SetZero();
  m_bSpawnPrefabOnStatic = false;
}

ezProjectileComponent::~ezProjectileComponent() = default;

void ezProjectileComponent::Update()
{
  if (m_fGravityMultiplier == 0.0f && m_fMetersPerSecond == 0.0f)
    return;

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

    ezPhysicsQueryParameters queryParams(m_uiCollisionLayer);
    queryParams.m_bIgnoreInitialOverlap = true;
    queryParams.m_ShapeTypes = m_ShapeTypesToHit;

    ezPhysicsCastResult castResult;
    if (pPhysicsInterface->Raycast(castResult, pEntity->GetGlobalPosition(), vCurDirection, fDistance, queryParams))
    {
      const ezSurfaceResourceHandle hSurface = castResult.m_hSurface.IsValid() ? castResult.m_hSurface : m_hFallbackSurface;

      const ezInt32 iInteraction = FindSurfaceInteraction(hSurface);

      if (iInteraction == -1)
      {
        GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
        vNewPosition = castResult.m_vPosition;
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
          if (interaction.m_fImpulse > 0.0f)
          {
            if (GetWorld()->TryGetObject(castResult.m_hActorObject, pObject))
            {
              ezMsgPhysicsAddImpulse msg;
              msg.m_vGlobalPosition = castResult.m_vPosition;
              msg.m_vImpulse = vCurDirection * interaction.m_fImpulse;
              msg.m_uiObjectFilterID = castResult.m_uiObjectFilterID;
              msg.m_pInternalPhysicsShape = castResult.m_pInternalPhysicsShape;
              msg.m_pInternalPhysicsActor = castResult.m_pInternalPhysicsActor;

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
          vNewPosition = castResult.m_vPosition;
        }
        else if (interaction.m_Reaction == ezProjectileReaction::Reflect || interaction.m_Reaction == ezProjectileReaction::Bounce)
        {
          /// \todo Should reflect around the actual hit position
          /// \todo Should preserve travel distance while reflecting

          // const float fLength = (vPos - pEntity->GetGlobalPosition()).GetLength();

          vNewPosition = pEntity->GetGlobalPosition(); // vPos;

          const ezVec3 vNewDirection = vCurDirection.GetReflectedVector(castResult.m_vNormal);

          ezQuat qRot = ezQuat::MakeShortestRotation(vCurDirection, vNewDirection);

          GetOwner()->SetGlobalRotation(qRot * GetOwner()->GetGlobalRotation());

          m_vVelocity = qRot * m_vVelocity;

          if (interaction.m_Reaction == ezProjectileReaction::Bounce)
          {
            ezResourceLock<ezSurfaceResource> pSurface(hSurface, ezResourceAcquireMode::BlockTillLoaded);

            if (pSurface)
            {
              m_vVelocity *= pSurface->GetDescriptor().m_fPhysicsRestitution;
            }

            if (m_vVelocity.GetLength() < 1.0f)
            {
              m_vVelocity = ezVec3::MakeZero();
              m_fGravityMultiplier = 0.0f;

              if (m_bSpawnPrefabOnStatic)
              {
                SpawnDeathPrefab();
                GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
              }
            }
          }
        }
        else if (interaction.m_Reaction == ezProjectileReaction::Attach)
        {
          m_fMetersPerSecond = 0.0f;
          m_fGravityMultiplier = 0.0f;
          vNewPosition = castResult.m_vPosition;

          ezGameObject* pObject;
          if (GetWorld()->TryGetObject(castResult.m_hActorObject, pObject))
          {
            pObject->AddChild(GetOwner()->GetHandle(), ezTransformPreservation::Enum::PreserveGlobal);
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
  }

  // Version 5
  s << m_ShapeTypesToHit;

  // Version 6
  s << m_bSpawnPrefabOnStatic;
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
  }

  if (uiVersion >= 5)
  {
    s >> m_ShapeTypesToHit;
  }

  if (uiVersion >= 6)
  {
    s >> m_bSpawnPrefabOnStatic;
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

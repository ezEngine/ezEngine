#include <PCH.h>
#include <GameEngine/Components/ProjectileComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/TriggerMessage.h>
#include <GameEngine/Prefabs/PrefabResource.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezProjectileReaction, 1)
EZ_ENUM_CONSTANT(ezProjectileReaction::Absorb),
EZ_ENUM_CONSTANT(ezProjectileReaction::Reflect),
EZ_ENUM_CONSTANT(ezProjectileReaction::Attach),
EZ_ENUM_CONSTANT(ezProjectileReaction::PassThrough)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezProjectileSurfaceInteraction, ezNoBase, 1, ezRTTIDefaultAllocator<ezProjectileSurfaceInteraction>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Surface", GetSurface, SetSurface)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
    EZ_ENUM_MEMBER_PROPERTY("Reaction", ezProjectileReaction, m_Reaction),
    EZ_MEMBER_PROPERTY("Interaction", m_sInteraction),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE

EZ_BEGIN_COMPONENT_TYPE(ezProjectileComponent, 2)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Speed", m_fMetersPerSecond)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("GravityMultiplier", m_fGravityMultiplier),
    EZ_MEMBER_PROPERTY("MaxLifetime", m_MaxLifetime)->AddAttributes(new ezClampValueAttribute(ezTime(), ezVariant())),
    EZ_ACCESSOR_PROPERTY("OnTimeoutSpawn", GetTimeoutPrefab, SetTimeoutPrefab)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_ARRAY_MEMBER_PROPERTY("Interactions", m_SurfaceInteractions),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezInternalComponentMessage, OnTriggered),
  }
  EZ_END_MESSAGEHANDLERS
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.2f, ezColor::OrangeRed),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE


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
  ezPhysicsWorldModuleInterface* pPhysicsInterface = GetManager()->m_pPhysicsInterface;

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
      const ezInt32 iInteraction = FindSurfaceInteraction(hitResult.m_hSurface);

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
          TriggerSurfaceInteraction(hitResult.m_hSurface, hitResult.m_vPosition, hitResult.m_vNormal, vCurDirection, interaction.m_sInteraction);
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

          //const float fLength = (vPos - pEntity->GetGlobalPosition()).GetLength();

          vNewPosition = pEntity->GetGlobalPosition();// vPos;

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
          if (GetWorld()->TryGetObject(hitResult.m_hGameObject, pObject))
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

  s << m_SurfaceInteractions.GetCount();
  for (const auto& ia : m_SurfaceInteractions)
  {
    s << ia.m_hSurface;

    ezProjectileReaction::StorageType storage = ia.m_Reaction;
    s << storage;

    s << ia.m_sInteraction;
  }
}

void ezProjectileComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fMetersPerSecond;
  s >> m_fGravityMultiplier;
  s >> m_uiCollisionLayer;
  s >> m_MaxLifetime;
  s >> m_hTimeoutPrefab;

  ezUInt32 count;
  s >> count;
  m_SurfaceInteractions.SetCount(count);
  for (ezUInt32 i = 0; i < count; ++i)
  {
    s >> m_SurfaceInteractions[i].m_hSurface;

    ezProjectileReaction::StorageType storage = 0;
    s >> storage;
    m_SurfaceInteractions[i].m_Reaction = (ezProjectileReaction::Enum)storage;

    s >> m_SurfaceInteractions[i].m_sInteraction;
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
      ezResourceLock<ezSurfaceResource> pSurf(hCurSurf, ezResourceAcquireMode::NoFallback);
      hCurSurf = pSurf->GetDescriptor().m_hBaseSurface;
    }
  }

  return -1;
}


void ezProjectileComponent::TriggerSurfaceInteraction(const ezSurfaceResourceHandle& hSurface, const ezVec3& vPos, const ezVec3& vNormal, const ezVec3& vDirection, const char* szInteraction)
{
  ezResourceLock<ezSurfaceResource> pSurface(hSurface, ezResourceAcquireMode::NoFallback);
  pSurface->InteractWithSurface(GetWorld(), vPos, vNormal, vDirection, ezTempHashedString(szInteraction));
}


void ezProjectileComponent::OnSimulationStarted()
{
  if (m_MaxLifetime.GetSeconds() > 0.0)
  {
    ezInternalComponentMessage msg;
    msg.m_uiUsageStringHash = ezTempHashedString::ComputeHash("Suicide");

    PostMessage(msg, ezObjectMsgQueueType::NextFrame, m_MaxLifetime);

    // make sure the prefab is available when the projectile dies
    if (m_hTimeoutPrefab.IsValid())
    {
      ezResourceManager::PreloadResource(m_hTimeoutPrefab, m_MaxLifetime);
    }
  }

  m_vVelocity = GetOwner()->GetDirForwards() * m_fMetersPerSecond;
}

void ezProjectileComponent::OnTriggered(ezInternalComponentMessage& msg)
{
  if (msg.m_uiUsageStringHash != ezTempHashedString::ComputeHash("Suicide"))
    return;

  if (m_hTimeoutPrefab.IsValid())
  {
    ezResourceLock<ezPrefabResource> pPrefab(m_hTimeoutPrefab);

    pPrefab->InstantiatePrefab(*GetWorld(), GetOwner()->GetGlobalTransform());
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

//////////////////////////////////////////////////////////////////////////


ezProjectileComponentManager::ezProjectileComponentManager(ezWorld* pWorld)
  : ezComponentManagerSimple<class ezProjectileComponent, ezComponentUpdateType::WhenSimulating>(pWorld)
  , m_pPhysicsInterface(nullptr)
{

}

void ezProjectileComponentManager::Initialize()
{
  ezComponentManagerSimple<ezProjectileComponent, ezComponentUpdateType::WhenSimulating>::Initialize();

  m_pPhysicsInterface = GetWorld()->GetModuleOfBaseType<ezPhysicsWorldModuleInterface>();
}


void ezProjectileComponentManager::SimpleUpdate(const ezWorldModule::UpdateContext& context)
{
  if (m_pPhysicsInterface == nullptr)
  {
    m_pPhysicsInterface = GetWorld()->GetModuleOfBaseType<ezPhysicsWorldModuleInterface>();
  }

  SUPER::SimpleUpdate(context);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezProjectileComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezProjectileComponentPatch_1_2()
    : ezGraphPatch(ezGetStaticRTTI<ezProjectileComponent>(), 2) {}

  virtual void Patch(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Gravity Multiplier", "GravityMultiplier");
    pNode->RenameProperty("Max Lifetime", "MaxLifetime");
    pNode->RenameProperty("Timeout Prefab", "TimeoutPrefab");
    pNode->RenameProperty("Collision Layer", "CollisionLayer");
  }
};

ezProjectileComponentPatch_1_2 g_ezProjectileComponentPatch_1_2;

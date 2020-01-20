
#include <GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/RaycastComponent.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRaycastComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("DisableTargetObjectOnNoHit", m_bDisableTargetObjectOnNoHit)->AddAttributes(new ezDefaultValueAttribute(false)),
    EZ_ACCESSOR_PROPERTY("RaycastEndObject", DummyGetter, SetRaycastEndObject)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_MEMBER_PROPERTY("CollisionLayerEndPoint", m_uiCollisionLayerEndPoint)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("CollisionLayerTrigger", m_uiCollisionLayerTrigger)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_ACCESSOR_PROPERTY("TriggerMessage", GetTriggerMessage, SetTriggerMessage)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.2f, ezColor::YellowGreen),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRaycastComponent::ezRaycastComponent() = default;

ezRaycastComponent::~ezRaycastComponent() = default;


void ezRaycastComponent::OnSimulationStarted()
{
  m_pPhysicsWorldModule = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>();
  m_hLastTriggerObjectInRay.Invalidate();

  ezGameObject* pEndObject = nullptr;
  if (GetWorld()->TryGetObject(m_hRaycastEndObject, pEndObject))
  {
    if (!pEndObject->IsDynamic())
    {
      pEndObject->MakeDynamic();
    }
  }
}

void ezRaycastComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  stream.WriteGameObjectHandle(m_hRaycastEndObject);
  s << m_fMaxDistance;
  s << m_bDisableTargetObjectOnNoHit;
  s << m_uiCollisionLayerEndPoint;
  s << m_uiCollisionLayerTrigger;
  s << m_sTriggerMessage;
}

void ezRaycastComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  auto& s = stream.GetStream();

  m_hRaycastEndObject = stream.ReadGameObjectHandle();
  s >> m_fMaxDistance;
  s >> m_bDisableTargetObjectOnNoHit;
  s >> m_uiCollisionLayerEndPoint;
  s >> m_uiCollisionLayerTrigger;
  s >> m_sTriggerMessage;
}


void ezRaycastComponent::SetTriggerMessage(const char* sz)
{
  m_sTriggerMessage.Assign(sz);
}

const char* ezRaycastComponent::GetTriggerMessage() const
{
  return m_sTriggerMessage.GetData();
}

void ezRaycastComponent::SetRaycastEndObject(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hRaycastEndObject = resolver(szReference, GetHandle(), "RaycastEndObject");
}

void ezRaycastComponent::Update()
{
  if (m_hRaycastEndObject.IsInvalidated())
    return;

  ezGameObject* pEndObject = nullptr;
  if (!GetWorld()->TryGetObject(m_hRaycastEndObject, pEndObject))
  {
    return;
  }

  if (!m_pPhysicsWorldModule)
  {
    // Happens in Prefab viewports
    return;
  }

  const ezVec3 rayStartPosition = GetOwner()->GetGlobalPosition();
  const ezVec3 rayDir = GetOwner()->GetGlobalDirForwards();

  //ezDebugRenderer::Line lines[] = { {rayStartPosition, rayStartPosition + rayDir * m_fMaxDistance} };
  //ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::RebeccaPurple);

  float fHitDistance = m_fMaxDistance;
  ezPhysicsHitResult hit;
  if (m_pPhysicsWorldModule->CastRay(rayStartPosition, rayDir, m_fMaxDistance, m_uiCollisionLayerEndPoint, hit))
  {
    fHitDistance = hit.m_fDistance;

    if (!pEndObject->GetActiveFlag() && m_bDisableTargetObjectOnNoHit)
    {
      pEndObject->SetActiveFlag(true);
    }
  }
  else
  {
    if (m_bDisableTargetObjectOnNoHit)
    {
      pEndObject->SetActiveFlag(false);
    }
    else
    {
      if (!pEndObject->GetActiveFlag())
      {
        pEndObject->SetActiveFlag(true);
      }
    }
  }

  if (!m_sTriggerMessage.IsEmpty() && m_uiCollisionLayerEndPoint != m_uiCollisionLayerTrigger)
  {
    ezPhysicsHitResult triggerHit;
    if (m_pPhysicsWorldModule->CastRay(rayStartPosition, rayDir, fHitDistance, m_uiCollisionLayerTrigger, triggerHit) && triggerHit.m_fDistance < fHitDistance)
    {
      // We have a hit, check the objects
      if (m_hLastTriggerObjectInRay != triggerHit.m_hActorObject)
      {
        // If we had another object, we now have one closer - send
        // deactivated for the old object and activate the new one
        if (!m_hLastTriggerObjectInRay.IsInvalidated())
        {
          PostTriggerMessage(ezTriggerState::Deactivated, m_hLastTriggerObjectInRay);
        }

        // Activate the new hit
        m_hLastTriggerObjectInRay = triggerHit.m_hActorObject;
        PostTriggerMessage(ezTriggerState::Activated, m_hLastTriggerObjectInRay);
      }
      // If it is still the same object as before we send a continuing message
      else
      {
        PostTriggerMessage(ezTriggerState::Continuing, m_hLastTriggerObjectInRay);
      }
    }
    else
    {
      // No hit anymore?
      if (!m_hLastTriggerObjectInRay.IsInvalidated())
      {
        PostTriggerMessage(ezTriggerState::Deactivated, m_hLastTriggerObjectInRay);
      }

      m_hLastTriggerObjectInRay.Invalidate();
    }
  }

  pEndObject->SetGlobalPosition(rayStartPosition + fHitDistance * rayDir);
}

void ezRaycastComponent::PostTriggerMessage(ezTriggerState::Enum state, ezGameObjectHandle hObject)
{
  ezMsgTriggerTriggered msg;

  msg.m_TriggerState = state;
  msg.m_uiMessageStringHash = m_sTriggerMessage.GetHash();
  msg.m_hTriggeringObject = hObject;

  m_TriggerEventSender.PostMessage(msg, this, GetOwner(), ezObjectMsgQueueType::PostTransform);
}


#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Gameplay/RaycastComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

ezRaycastComponentManager::ezRaycastComponentManager(ezWorld* pWorld)
  : SUPER(pWorld)
{
}

void ezRaycastComponentManager::Initialize()
{
  // we want to do the raycast as late as possible, ie. after animated objects and characters moved
  // such that we get the latest position that is in sync with those animated objects
  // therefore we move the update into the post async phase and set a low priority (low = updated late)
  // we DO NOT want to use post transform update, because when we move the target object
  // child objects of the target node should still get the full global transform update within this frame

  auto desc = ezWorldModule::UpdateFunctionDesc(ezWorldModule::UpdateFunction(&ezRaycastComponentManager::Update, this), "ezRaycastComponentManager::Update");
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase = UpdateFunctionDesc::Phase::PostAsync;
  desc.m_fPriority = -1000;

  this->RegisterUpdateFunction(desc);
}

void ezRaycastComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRaycastComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("DisableTargetObjectOnNoHit", m_bDisableTargetObjectOnNoHit),
    EZ_ACCESSOR_PROPERTY("RaycastEndObject", DummyGetter, SetRaycastEndObject)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_MEMBER_PROPERTY("ForceTargetParentless", m_bForceTargetParentless),
    EZ_BITFLAGS_MEMBER_PROPERTY("ShapeTypesToHit", ezPhysicsShapeType, m_ShapeTypesToHit)->AddAttributes(new ezDefaultValueAttribute(ezVariant(ezPhysicsShapeType::Default & ~(ezPhysicsShapeType::Trigger)))),
    EZ_MEMBER_PROPERTY("CollisionLayerEndPoint", m_uiCollisionLayerEndPoint)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("CollisionLayerTrigger", m_uiCollisionLayerTrigger)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_ACCESSOR_PROPERTY("TriggerMessage", GetTriggerMessage, SetTriggerMessage),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.5f, ezColor::YellowGreen),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRaycastComponent::ezRaycastComponent() = default;
ezRaycastComponent::~ezRaycastComponent() = default;

void ezRaycastComponent::Deinitialize()
{
  if (m_bForceTargetParentless)
  {
    // see end of ezRaycastComponent::Update() for details
    GetWorld()->DeleteObjectDelayed(m_hRaycastEndObject);
  }

  SUPER::Deinitialize();
}

void ezRaycastComponent::OnActivated()
{
  SUPER::OnActivated();
}

void ezRaycastComponent::OnDeactivated()
{
  if (m_bDisableTargetObjectOnNoHit && m_bForceTargetParentless)
  {
    ezGameObject* pEndObject = nullptr;
    if (GetWorld()->TryGetObject(m_hRaycastEndObject, pEndObject))
    {
      pEndObject->SetActiveFlag(false);
    }
  }

  SUPER::OnDeactivated();
}

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

void ezRaycastComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  inout_stream.WriteGameObjectHandle(m_hRaycastEndObject);
  s << m_fMaxDistance;
  s << m_bDisableTargetObjectOnNoHit;
  s << m_uiCollisionLayerEndPoint;
  s << m_uiCollisionLayerTrigger;
  s << m_sTriggerMessage;
  s << m_bForceTargetParentless;
  s << m_ShapeTypesToHit;
}

void ezRaycastComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  m_hRaycastEndObject = inout_stream.ReadGameObjectHandle();
  s >> m_fMaxDistance;
  s >> m_bDisableTargetObjectOnNoHit;
  s >> m_uiCollisionLayerEndPoint;
  s >> m_uiCollisionLayerTrigger;
  s >> m_sTriggerMessage;

  if (uiVersion >= 2)
  {
    s >> m_bForceTargetParentless;
  }

  if (uiVersion >= 3)
  {
    s >> m_ShapeTypesToHit;
  }
}

void ezRaycastComponent::SetTriggerMessage(const char* szSz)
{
  m_sTriggerMessage.Assign(szSz);
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

  if (!m_pPhysicsWorldModule)
  {
    // Happens in Prefab viewports
    return;
  }

  ezGameObject* pEndObject = nullptr;
  if (!GetWorld()->TryGetObject(m_hRaycastEndObject, pEndObject))
  {
    // early out in the future
    m_hRaycastEndObject.Invalidate();
    return;
  }

  // if the owner object moved this frame, we want the latest global position as the ray starting position
  // this is especially important when the raycast component is attached to something that animates
  GetOwner()->UpdateGlobalTransform();

  const ezVec3 rayStartPosition = GetOwner()->GetGlobalPosition();
  const ezVec3 rayDir = GetOwner()->GetGlobalDirForwards().GetNormalized(); // PhysX is very picky about normalized vectors

  float fHitDistance = m_fMaxDistance;
  ezPhysicsCastResult hit;

  {
    ezPhysicsQueryParameters queryParams(m_uiCollisionLayerEndPoint);
    queryParams.m_bIgnoreInitialOverlap = true;
    queryParams.m_ShapeTypes = m_ShapeTypesToHit;

    if (m_pPhysicsWorldModule->Raycast(hit, rayStartPosition, rayDir, m_fMaxDistance, queryParams))
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
  }

  if (false)
  {
    ezDebugRenderer::Line lines[] = {{rayStartPosition, rayStartPosition + rayDir * fHitDistance}};
    ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::GreenYellow);
  }

  if (!m_sTriggerMessage.IsEmpty() && m_uiCollisionLayerEndPoint != m_uiCollisionLayerTrigger)
  {
    ezPhysicsCastResult triggerHit;
    ezPhysicsQueryParameters queryParams2(m_uiCollisionLayerTrigger);
    queryParams2.m_bIgnoreInitialOverlap = true;
    queryParams2.m_ShapeTypes = m_ShapeTypesToHit;

    if (m_pPhysicsWorldModule->Raycast(triggerHit, rayStartPosition, rayDir, fHitDistance, queryParams2) && triggerHit.m_fDistance < fHitDistance)
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

  if (m_bForceTargetParentless)
  {
    // this is necessary to ensure perfect positioning when the target is originally attached to a moving object
    // that happens, for instance, when the target is part of a prefab, which includes the raycast component, of course
    // and the prefab is then attached to e.g. a character
    // without detaching the target object from all parents, it is not possible to ensure that it will never deviate from the
    // position set by the raycast component
    // since we now change ownership (target is not deleted with its former parent anymore)
    // this flag also means that the raycast component will delete the target object, when it dies
    pEndObject->SetParent(ezGameObjectHandle());
  }

  pEndObject->SetGlobalPosition(rayStartPosition + fHitDistance * rayDir);
}

void ezRaycastComponent::PostTriggerMessage(ezTriggerState::Enum state, ezGameObjectHandle hObject)
{
  ezMsgTriggerTriggered msg;

  msg.m_TriggerState = state;
  msg.m_sMessage = m_sTriggerMessage;
  msg.m_hTriggeringObject = hObject;

  m_TriggerEventSender.PostEventMessage(msg, this, GetOwner(), ezTime::MakeZero(), ezObjectMsgQueueType::PostTransform);
}


EZ_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Gameplay_Implementation_RaycastComponent);

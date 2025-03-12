
#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/CommonMessages.h>
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

  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezRaycastComponentManager::Update, this);
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase = ezWorldUpdatePhase::PostAsync;
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

////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRaycastComponent, 4, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("DisableTargetObjectOnNoHit", m_bDisableTargetObjectOnNoHit),
    EZ_ACCESSOR_PROPERTY("RaycastEndObject", DummyGetter, SetRaycastEndObject)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_MEMBER_PROPERTY("ForceTargetParentless", m_bForceTargetParentless),
    EZ_BITFLAGS_MEMBER_PROPERTY("ShapeTypesToHit", ezPhysicsShapeType, m_ShapeTypesToHit)->AddAttributes(new ezDefaultValueAttribute(ezVariant(ezPhysicsShapeType::Default & ~(ezPhysicsShapeType::Trigger)))),
    EZ_MEMBER_PROPERTY("CollisionLayerEndPoint", m_uiCollisionLayerEndPoint)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("ChangeNotificationMsg", m_sChangeNotificationMsg),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GetCurrentDistance),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetCurrentEndPosition),
    EZ_SCRIPT_FUNCTION_PROPERTY(HasHit),
  }
  EZ_END_FUNCTIONS;
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

void ezRaycastComponent::OnDeactivated()
{
  if (m_bDisableTargetObjectOnNoHit)
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
  s << m_bForceTargetParentless;
  s << m_ShapeTypesToHit;
  s << m_sChangeNotificationMsg;
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

  if (uiVersion < 4)
  {
    ezUInt8 uiCollisionLayerTrigger = 0;
    s >> uiCollisionLayerTrigger;

    ezStringBuilder sTriggerMessage;
    s >> sTriggerMessage;
  }

  if (uiVersion >= 2)
  {
    s >> m_bForceTargetParentless;
  }

  if (uiVersion >= 3)
  {
    s >> m_ShapeTypesToHit;
  }

  if (uiVersion >= 4)
  {
    s >> m_sChangeNotificationMsg;
  }
}

ezVec3 ezRaycastComponent::GetCurrentEndPosition() const
{
  return GetOwner()->GetGlobalPosition() + m_fCurrentDistance * GetOwner()->GetGlobalDirForwards();
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

  bool bAnyChange = false;

  const ezVec3 rayStartPosition = GetOwner()->GetGlobalPosition();
  const ezVec3 rayDir = GetOwner()->GetGlobalDirForwards().GetNormalized(); // PhysX is very picky about normalized vectors

  ezPhysicsCastResult hit;

  {
    ezPhysicsQueryParameters queryParams(m_uiCollisionLayerEndPoint);
    queryParams.m_bIgnoreInitialOverlap = true;
    queryParams.m_ShapeTypes = m_ShapeTypesToHit;

    if (m_pPhysicsWorldModule->Raycast(hit, rayStartPosition, rayDir, m_fMaxDistance, queryParams))
    {
      if (!pEndObject->GetActiveFlag() && m_bDisableTargetObjectOnNoHit)
      {
        pEndObject->SetActiveFlag(true);
        bAnyChange = true;
      }

      if (!ezMath::IsEqual(m_fCurrentDistance, hit.m_fDistance, 0.001f))
      {
        m_fCurrentDistance = hit.m_fDistance;
        bAnyChange = true;
      }
    }
    else
    {
      if (m_fCurrentDistance != m_fMaxDistance)
      {
        m_fCurrentDistance = m_fMaxDistance;
        bAnyChange = true;
      }

      if (m_bDisableTargetObjectOnNoHit)
      {
        if (pEndObject->GetActiveFlag())
        {
          pEndObject->SetActiveFlag(false);
          bAnyChange = true;
        }
      }
      else
      {
        if (!pEndObject->GetActiveFlag())
        {
          pEndObject->SetActiveFlag(true);
          bAnyChange = true;
        }
      }
    }
  }

  if (m_bForceTargetParentless && GetUserFlag(0) == false)
  {
    // only do this once
    SetUserFlag(0, true);

    // this is necessary to ensure perfect positioning when the target is originally attached to a moving object
    // that happens, for instance, when the target is part of a prefab, which includes the raycast component, of course
    // and the prefab is then attached to e.g. a character
    // without detaching the target object from all parents, it is not possible to ensure that it will never deviate from the
    // position set by the raycast component
    // since we now change ownership (target is not deleted with its former parent anymore)
    // this flag also means that the raycast component will delete the target object, when it dies
    pEndObject->SetParent(ezGameObjectHandle());
  }

  const ezVec3 vOldPos = pEndObject->GetGlobalPosition();
  const ezVec3 vNewPos = rayStartPosition + m_fCurrentDistance * rayDir;

  if (!vOldPos.IsEqual(vNewPos, 0.001f))
  {
    pEndObject->SetGlobalPosition(vNewPos);
    bAnyChange = true;
  }

  if (!m_sChangeNotificationMsg.IsEmpty() && bAnyChange)
  {
    ezMsgGenericEvent msg;
    msg.m_sMessage = m_sChangeNotificationMsg;

    GetOwner()->SendEventMessage(msg, this);
  }


  if (false)
  {
    ezDebugRendererLine lines[] = {{rayStartPosition, vNewPos}};
    ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::GreenYellow);
  }
}

EZ_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Gameplay_Implementation_RaycastComponent);

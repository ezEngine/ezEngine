#include <PhysXPlugin/PhysXPluginPCH.h>

#include <PhysXPlugin/Components/PxObjectGrabComponent.h>
#include <PhysXPlugin/Joints/Px6DOFJointComponent.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxGrabObjectComponent, 1, ezComponentMode::Static)
{
  //EZ_BEGIN_PROPERTIES
  //{
  //  EZ_ACCESSOR_PROPERTY("Kinematic", GetKinematic, SetKinematic),
  //}
  //EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(TryGrabObject),
    EZ_SCRIPT_FUNCTION_PROPERTY(ReleaseGrabbedObject),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Special"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on


ezPxGrabObjectComponent::ezPxGrabObjectComponent()
{
}

void ezPxGrabObjectComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();
}

void ezPxGrabObjectComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();
}

static ezPxDynamicActorComponent* FindActor(ezGameObject* pObject)
{
  while (pObject != nullptr)
  {
    ezPxDynamicActorComponent* pActor;
    if (pObject->TryGetComponentOfBaseType(pActor))
    {
      if (pActor->GetKinematic())
        return nullptr;

      return pActor;
    }

    pObject = pObject->GetParent();
  }

  return nullptr;
}

bool ezPxGrabObjectComponent::TryGrabObject()
{
  if (!m_hJoint.IsInvalidated())
  {
    ReleaseGrabbedObject();
    return false;
  }

  auto pOwner = GetOwner();

  auto marker = ezSpatialData::RegisterCategory("GrabObjectMarker");

  ezHybridArray<ezGameObject*, 16> objects;

  GetWorld()->GetSpatialSystem()->FindObjectsInSphere(ezBoundingSphere(pOwner->GetGlobalPosition(), 1.0f), marker.GetBitmask(), objects);

  if (objects.IsEmpty())
    return false;

  const ezVec3 vOwnPos = pOwner->GetGlobalPosition();

  float fClosest = ezMath::MaxValue<float>();
  ezGameObject* pClosestMarker = nullptr;
  ezPxDynamicActorComponent* pClosestActor = nullptr;

  for (auto pObj : objects)
  {
    const float fDist = (vOwnPos - pObj->GetGlobalPosition()).GetLengthSquared();

    if (fDist < fClosest)
    {
      if (auto pActor = FindActor(pObj))
      {
        pClosestMarker = pObj;
        pClosestActor = pActor;
        fClosest = fDist;
      }
    }
  }

  if (pClosestActor == nullptr)
    return false;

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  ezPxDynamicActorComponent* pParentActor = nullptr;
  {
    if (!pOwner->TryGetComponentOfBaseType(pParentActor))
      return false;

    if (!pParentActor->GetKinematic())
      return false;
  }

  m_hGrabbedActor = pClosestActor->GetHandle();
  m_fPrevMass = pClosestActor->GetMass();
  pClosestActor->SetMass(0.1f);

  {
    ezPx6DOFJointComponent* pJoint = nullptr;
    m_hJoint = GetWorld()->GetOrCreateComponentManager<ezPx6DOFJointComponentManager>()->CreateComponent(pOwner, pJoint);

    pJoint->SetFreeAngularAxis(ezPxAxis::None);
    pJoint->SetFreeLinearAxis(ezPxAxis::X | ezPxAxis::Y | ezPxAxis::Z);
    pJoint->SetLinearLimitMode(ezPxJointLimitMode::SoftLimit);
    pJoint->SetLinearStiffness(50.0f);
    pJoint->SetLinearDamping(10.0f);

    pJoint->SetParentActor(pParentActor->GetOwner()->GetHandle());
    pJoint->SetChildActor(pClosestActor->GetOwner()->GetHandle());
    pJoint->SetChildActorAnchor(pClosestActor->GetOwner()->GetHandle());

    pClosestActor->GetActor()->getSolverIterationCounts(m_uiPrevPosIterations, m_uiPrevVelIterations);
    pClosestActor->GetActor()->setSolverIterationCounts(16, 16);
  }

  return true;
}

void ezPxGrabObjectComponent::ReleaseGrabbedObject()
{
  if (m_hJoint.IsInvalidated())
    return;

  ezPxDynamicActorComponent* pActor;
  if (GetWorld()->TryGetComponent(m_hGrabbedActor, pActor))
  {
    pActor->SetMass(m_fPrevMass); // TODO: not sure that's the best way to reset the mass
    pActor->GetActor()->setSolverIterationCounts(m_uiPrevPosIterations, m_uiPrevVelIterations);
  }

  ezComponent* pComponent;
  if (GetWorld()->TryGetComponent(m_hJoint, pComponent))
  {
    pComponent->GetOwningManager()->DeleteComponent(pComponent);
  }

  m_hJoint.Invalidate();
}

#include <PhysXPluginPCH.h>

#include <Core/Messages/CollisionMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxCharacterProxyComponent.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>

using namespace physx;

namespace
{
  static ezBitflags<ezPxCharacterCollisionFlags> ConvertCollisionFlags(PxU32 pxFlags)
  {
    ezBitflags<ezPxCharacterCollisionFlags> result = ezPxCharacterCollisionFlags::None;

    if ((pxFlags & PxControllerCollisionFlag::eCOLLISION_SIDES) != 0)
    {
      result.Add(ezPxCharacterCollisionFlags::Sides);
    }

    if ((pxFlags & PxControllerCollisionFlag::eCOLLISION_UP) != 0)
    {
      result.Add(ezPxCharacterCollisionFlags::Above);
    }

    if ((pxFlags & PxControllerCollisionFlag::eCOLLISION_DOWN) != 0)
    {
      result.Add(ezPxCharacterCollisionFlags::Below);
    }

    return result;
  }

  class ezPxControllerBehaviorCallback : public PxControllerBehaviorCallback
  {
    virtual PxControllerBehaviorFlags getBehaviorFlags(const PxShape& shape, const PxActor& actor) override
    {
      const PxRigidDynamic* pDynamicRigidBody = actor.is<PxRigidDynamic>();
      if (pDynamicRigidBody != nullptr && pDynamicRigidBody->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
      {
        return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT | PxControllerBehaviorFlag::eCCT_SLIDE;
      }

      return PxControllerBehaviorFlags(0);
    }

    virtual PxControllerBehaviorFlags getBehaviorFlags(const PxController& controller) override { return PxControllerBehaviorFlags(0); }

    virtual PxControllerBehaviorFlags getBehaviorFlags(const PxObstacle& obstacle) override { return PxControllerBehaviorFlags(0); }
  };

  class ezPxControllerHitCallback : public PxUserControllerHitReport
  {
    virtual void onShapeHit(const PxControllerShapeHit& hit) override
    {
      PxRigidDynamic* pDynamicRigidBody = hit.actor->is<PxRigidDynamic>();
      if (pDynamicRigidBody != nullptr && !pDynamicRigidBody->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
      {
        ezPxCharacterProxyComponent* pCharacterProxyComponent = ezPxUserData::GetCharacterProxyComponent(hit.controller->getUserData());
        ezPxDynamicActorComponent* pDynamicActorComponent = ezPxUserData::GetDynamicActorComponent(hit.actor->userData);

        if (pCharacterProxyComponent == nullptr || pDynamicActorComponent == nullptr)
          return;

        ezGameObject* pCharacterObject = pCharacterProxyComponent->GetOwner();
        const float fMass = hit.controller->getActor()->getMass();

        ezMsgCollision msg;

        msg.m_hObjectA = pCharacterObject->GetHandle();
        msg.m_hObjectB = pDynamicActorComponent->GetOwner()->GetHandle();

        msg.m_hComponentA = pCharacterProxyComponent->GetHandle();
        msg.m_hComponentB = pDynamicActorComponent->GetHandle();

        msg.m_vPosition = ezPxConversionUtils::ToVec3(hit.worldPos);
        msg.m_vNormal = ezPxConversionUtils::ToVec3(hit.worldNormal);
        msg.m_vImpulse = ezPxConversionUtils::ToVec3(hit.dir) * fMass;

        pCharacterObject->SendMessage(msg);
      }
    }

    virtual void onControllerHit(const PxControllersHit& hit) override
    {
      // do nothing for now
    }

    virtual void onObstacleHit(const PxControllerObstacleHit& hit) override
    {
      // do nothing for now
    }
  };
} // namespace

//////////////////////////////////////////////////////////////////////////

struct ezPxCharacterProxyData
{
  ezPxQueryFilter m_QueryFilter;

  ezPxControllerBehaviorCallback m_BehaviorCallback;
  ezPxControllerHitCallback m_HitCallback;

  PxControllerFilters m_ControllerFilter;
  PxFilterData m_FilterData;
};

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezPxCharacterCollisionFlags, 1)
  EZ_ENUM_CONSTANT(ezPxCharacterCollisionFlags::None),
  EZ_ENUM_CONSTANT(ezPxCharacterCollisionFlags::Sides),
  EZ_ENUM_CONSTANT(ezPxCharacterCollisionFlags::Above),
  EZ_ENUM_CONSTANT(ezPxCharacterCollisionFlags::Below),
EZ_END_STATIC_REFLECTED_BITFLAGS;

EZ_BEGIN_COMPONENT_TYPE(ezPxCharacterProxyComponent, 3, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
      EZ_MEMBER_PROPERTY("CapsuleHeight", m_fCapsuleHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 10.0f)),
      EZ_MEMBER_PROPERTY("CapsuleCrouchHeight", m_fCapsuleCrouchHeight)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.0f, 10.0f)),
      EZ_MEMBER_PROPERTY("CapsuleRadius", m_fCapsuleRadius)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.1f, 5.0f)),
      EZ_MEMBER_PROPERTY("Mass", m_fMass)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(0.01f, ezVariant())),
      EZ_MEMBER_PROPERTY("MaxStepHeight", m_fMaxStepHeight)->AddAttributes(new ezDefaultValueAttribute(0.3f), new ezClampValueAttribute(0.0f, 5.0f)),
      EZ_MEMBER_PROPERTY("MaxSlopeAngle", m_MaxClimbingSlope)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(40.0f)), new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(80.0f))),
      EZ_MEMBER_PROPERTY("ForceSlopeSliding", m_bForceSlopeSliding)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_MEMBER_PROPERTY("ConstrainedClimbMode", m_bConstrainedClimbingMode),
      EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
      EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Move, In, "motion", In, "crouch"),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsCrouching),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetCurrentCapsuleHeight),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetCollisionFlags),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsGrounded),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetShapeId),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
      new ezCapsuleManipulatorAttribute("CapsuleHeight", "CapsuleRadius"),
      new ezCapsuleVisualizerAttribute("CapsuleHeight", "CapsuleRadius"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezPxCharacterProxyComponent::ezPxCharacterProxyComponent()
{
  m_Data = EZ_DEFAULT_NEW(ezPxCharacterProxyData);
}

ezPxCharacterProxyComponent::~ezPxCharacterProxyComponent() = default;

void ezPxCharacterProxyComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fCapsuleHeight;
  s << m_fCapsuleRadius;
  s << m_fMass;
  s << m_fMaxStepHeight;
  s << m_MaxClimbingSlope;
  s << m_bForceSlopeSliding;
  s << m_bConstrainedClimbingMode;
  s << m_uiCollisionLayer;
  s << m_fCapsuleCrouchHeight;
}


void ezPxCharacterProxyComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fCapsuleHeight;
  s >> m_fCapsuleRadius;

  if (uiVersion >= 2)
  {
    s >> m_fMass;
  }

  s >> m_fMaxStepHeight;
  s >> m_MaxClimbingSlope;
  s >> m_bForceSlopeSliding;
  s >> m_bConstrainedClimbingMode;
  s >> m_uiCollisionLayer;

  if (uiVersion >= 3)
  {
    s >> m_fCapsuleCrouchHeight;
  }
}

void ezPxCharacterProxyComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
}

void ezPxCharacterProxyComponent::OnDeactivated()
{
  if (m_pController != nullptr)
  {
    EZ_PX_WRITE_LOCK(*(m_pController->getScene()));

    m_pController->release();
    m_pController = nullptr;
  }

  if (ezPhysXWorldModule* pModule = GetWorld()->GetModule<ezPhysXWorldModule>())
  {
    pModule->DeleteShapeId(m_uiShapeId);
    pModule->DeallocateUserData(m_uiUserDataIndex);
  }

  SUPER::OnDeactivated();
}

void ezPxCharacterProxyComponent::OnSimulationStarted()
{
  if (!IsActive())
    return;

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  m_uiShapeId = pModule->CreateShapeId();

  const ezVec3& pos = GetOwner()->GetGlobalPosition();

  ezCoordinateSystem coordSystem;
  GetWorld()->GetCoordinateSystem(pos, coordSystem);

  PxCapsuleControllerDesc cd;
  cd.position = ezPxConversionUtils::ToExVec3(pos);
  cd.upDirection = ezPxConversionUtils::ToVec3(coordSystem.m_vUpDir);
  cd.slopeLimit = ezMath::Cos(m_MaxClimbingSlope);
  cd.contactOffset = ezMath::Max(m_fCapsuleRadius * 0.1f, 0.01f);
  cd.stepOffset = m_fMaxStepHeight;
  cd.reportCallback = &(m_Data->m_HitCallback);
  cd.behaviorCallback = &(m_Data->m_BehaviorCallback);
  cd.nonWalkableMode = m_bForceSlopeSliding ? PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING
                                            : PxControllerNonWalkableMode::ePREVENT_CLIMBING;
  cd.material = ezPhysX::GetSingleton()->GetDefaultMaterial();

  ezPxUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);
  cd.userData = pUserData;

  cd.radius = ezMath::Max(m_fCapsuleRadius, 0.0f);
  cd.height = ezMath::Max(m_fCapsuleHeight, 0.0f);
  cd.climbingMode = m_bConstrainedClimbingMode ? PxCapsuleClimbingMode::eCONSTRAINED : PxCapsuleClimbingMode::eEASY;

  if (!cd.isValid())
  {
    ezLog::Error("The Character Controller configuration is invalid.");
    return;
  }

  // Setup filter data
  m_Data->m_FilterData = ezPhysX::CreateFilterData(m_uiCollisionLayer, m_uiShapeId);

  m_Data->m_ControllerFilter.mCCTFilterCallback = nullptr;
  m_Data->m_ControllerFilter.mFilterCallback = &(m_Data->m_QueryFilter);
  m_Data->m_ControllerFilter.mFilterData = &(m_Data->m_FilterData);
  m_Data->m_ControllerFilter.mFilterFlags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

  {
    EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));
    m_pController = static_cast<PxCapsuleController*>(pModule->GetCharacterManager()->createController(cd));
    EZ_ASSERT_DEV(m_pController != nullptr, "Failed to create character controller");

    PxRigidDynamic* pActor = m_pController->getActor();
    pActor->setMass(m_fMass);
    pActor->userData = pUserData;

    PxShape* pShape = nullptr;
    pActor->getShapes(&pShape, 1);
    pShape->setSimulationFilterData(m_Data->m_FilterData);
    pShape->setQueryFilterData(m_Data->m_FilterData);
    pShape->userData = pUserData;
  }
}

void ezPxCharacterProxyComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(ezBoundingSphere(ezVec3(0, 0, -m_fCapsuleHeight * 0.5f), m_fCapsuleRadius), ezInvalidSpatialDataCategory);
  msg.AddBounds(ezBoundingSphere(ezVec3(0, 0, m_fCapsuleHeight * 0.5f), m_fCapsuleRadius), ezInvalidSpatialDataCategory);
}

ezBitflags<ezPxCharacterCollisionFlags> ezPxCharacterProxyComponent::Move(const ezVec3& vMotion, bool bCrouch)
{
  if (m_pController != nullptr)
  {
    ezGameObject* pOwner = GetOwner();

    ezVec3 vOldPos = pOwner->GetGlobalPosition();
    const float fElapsedTime = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();

    EZ_PX_WRITE_LOCK(*(m_pController->getScene()));
    PxControllerCollisionFlags collisionFlags =
      m_pController->move(ezPxConversionUtils::ToVec3(vMotion), 0.0f, fElapsedTime, m_Data->m_ControllerFilter);

    ezVec3 vNewPos = ezPxConversionUtils::ToVec3(m_pController->getPosition());
    pOwner->SetGlobalPosition(vNewPos);

    if (m_bIsCrouching != bCrouch)
    {
      if (bCrouch)
      {
        m_bIsCrouching = true;
        m_pController->resize(m_fCapsuleCrouchHeight);
      }
      else
      {
        // little offset upwards, to get out of the ground, otherwise the CC is stuck in it
        /// \todo support different gravity directions
        ezVec3 vStandUpPos = vNewPos + ezVec3(0, 0, (m_fCapsuleHeight - m_fCapsuleCrouchHeight) * 0.5f + 0.01f);

        ezTransform t;
        t.SetIdentity();
        t.m_vPosition.Set(vStandUpPos.x, vStandUpPos.y, vStandUpPos.z);

        // make sure the character controller does not overlap with anything when standing up
        ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
        if (!pModule->OverlapTestCapsule(m_fCapsuleRadius, m_fCapsuleHeight, t,
              ezPhysicsQueryParameters(m_uiCollisionLayer, ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic, GetShapeId())))
        {
          m_bIsCrouching = false;
          m_pController->resize(m_fCapsuleHeight);
        }
      }
    }

    return ConvertCollisionFlags(collisionFlags);
  }

  return ezPxCharacterCollisionFlags::None;
}

ezBitflags<ezPxCharacterCollisionFlags> ezPxCharacterProxyComponent::GetCollisionFlags() const
{
  if (m_pController != nullptr)
  {
    PxControllerState state;
    m_pController->getState(state);

    return ConvertCollisionFlags(state.collisionFlags);
  }

  return ezPxCharacterCollisionFlags::None;
}

ezGameObjectHandle ezPxCharacterProxyComponent::GetTouchedShapeObject() const
{
  if (m_pController != nullptr)
  {
    PxControllerState state;
    m_pController->getState(state);

    if (state.touchedShape != nullptr)
    {
      if (ezComponent* pComponent = ezPxUserData::GetShapeComponent(state.touchedShape->userData))
      {
        return pComponent->GetOwner()->GetHandle();
      }
    }
  }

  return ezGameObjectHandle();
}

ezGameObjectHandle ezPxCharacterProxyComponent::GetTouchedActorObject() const
{
  if (m_pController != nullptr)
  {
    PxControllerState state;
    m_pController->getState(state);

    if (state.touchedActor != nullptr)
    {
      if (ezComponent* pComponent = ezPxUserData::GetComponent(state.touchedActor->userData))
      {
        return pComponent->GetOwner()->GetHandle();
      }
    }
  }

  return ezGameObjectHandle();
}

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Components_Implementation_PxCharacterProxyComponent);

#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/Messages/CollisionMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxCharacterCapsuleShapeComponent.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>

using namespace physx;

namespace
{
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
        ezPxCharacterShapeComponent* pCharacterShape = ezPxUserData::GetCharacterShapeComponent(hit.controller->getUserData());
        ezPxDynamicActorComponent* pDynamicActorComponent = ezPxUserData::GetDynamicActorComponent(hit.actor->userData);

        if (pCharacterShape == nullptr || pDynamicActorComponent == nullptr)
          return;

        ezGameObject* pCharacterObject = pCharacterShape->GetOwner();
        const float fMass = hit.controller->getActor()->getMass();

        ezMsgCollision msg;

        msg.m_hObjectA = pCharacterObject->GetHandle();
        msg.m_hObjectB = pDynamicActorComponent->GetOwner()->GetHandle();

        msg.m_hComponentA = pCharacterShape->GetHandle();
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

struct ezPxCharacterCapsuleShapeData
{
  ezPxQueryFilter m_QueryFilter;

  ezPxControllerBehaviorCallback m_BehaviorCallback;
  ezPxControllerHitCallback m_HitCallback;

  PxControllerFilters m_ControllerFilter;
  PxFilterData m_FilterData;
};

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxCharacterCapsuleShapeComponent, 4, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
      EZ_MEMBER_PROPERTY("CapsuleHeight", m_fCapsuleHeight)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 10.0f)),
      EZ_MEMBER_PROPERTY("CapsuleRadius", m_fCapsuleRadius)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.1f, 5.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
      EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
      new ezCapsuleVisualizerAttribute("CapsuleHeight", "CapsuleRadius", ezColor::MediumVioletRed, nullptr, ezVisualizerAnchor::NegZ),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezPxCharacterCapsuleShapeComponent::ezPxCharacterCapsuleShapeComponent()
{
  m_pData = EZ_DEFAULT_NEW(ezPxCharacterCapsuleShapeData);
}

ezPxCharacterCapsuleShapeComponent::~ezPxCharacterCapsuleShapeComponent() = default;

void ezPxCharacterCapsuleShapeComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fCapsuleHeight;
  s << m_fCapsuleRadius;
}

void ezPxCharacterCapsuleShapeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fCapsuleHeight;
  s >> m_fCapsuleRadius;
}

void ezPxCharacterCapsuleShapeComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(ezBoundingSphere(ezVec3(0, 0, m_fCapsuleRadius), m_fCapsuleRadius), ezInvalidSpatialDataCategory);
  msg.AddBounds(ezBoundingSphere(ezVec3(0, 0, m_fCapsuleHeight + m_fCapsuleRadius), m_fCapsuleRadius), ezInvalidSpatialDataCategory);
}

void ezPxCharacterCapsuleShapeComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  const ezVec3 footPos = GetOwner()->GetGlobalPosition();
  const ezVec3 centerPos = footPos + ezVec3(0, 0, m_fCapsuleRadius + m_fCapsuleHeight * 0.5f);

  ezCoordinateSystem coordSystem;
  GetWorld()->GetCoordinateSystem(centerPos, coordSystem);

  PxCapsuleControllerDesc cd;
  cd.position = ezPxConversionUtils::ToExVec3(centerPos);
  cd.upDirection = ezPxConversionUtils::ToVec3(coordSystem.m_vUpDir);
  cd.slopeLimit = ezMath::Cos(m_MaxClimbingSlope);
  cd.contactOffset = ezMath::Max(m_fCapsuleRadius * 0.1f, 0.01f);
  cd.stepOffset = m_fMaxStepHeight;
  cd.reportCallback = &(m_pData->m_HitCallback);
  cd.behaviorCallback = &(m_pData->m_BehaviorCallback);
  cd.nonWalkableMode = m_bForceSlopeSliding ? PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING : PxControllerNonWalkableMode::ePREVENT_CLIMBING;
  cd.material = ezPhysX::GetSingleton()->GetDefaultMaterial();
  cd.userData = GetUserData();

  cd.radius = ezMath::Max(m_fCapsuleRadius, 0.0f);
  cd.height = ezMath::Max(m_fCapsuleHeight, 0.0f);
  cd.climbingMode = m_bConstrainedClimbingMode ? PxCapsuleClimbingMode::eCONSTRAINED : PxCapsuleClimbingMode::eEASY;

  m_fCurrentHeightValue = m_fCapsuleHeight;

  if (!cd.isValid())
  {
    ezLog::Error("The Character Controller configuration is invalid.");
    return;
  }

  // Setup filter data
  m_pData->m_FilterData = ezPhysX::CreateFilterData(m_uiCollisionLayer, GetShapeId());

  m_pData->m_QueryFilter.m_bIncludeQueryShapes = false;

  m_pData->m_ControllerFilter.mCCTFilterCallback = nullptr;
  m_pData->m_ControllerFilter.mFilterCallback = &(m_pData->m_QueryFilter);
  m_pData->m_ControllerFilter.mFilterData = &(m_pData->m_FilterData);
  m_pData->m_ControllerFilter.mFilterFlags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

  {
    EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));
    m_pController = static_cast<PxCapsuleController*>(pModule->GetCharacterManager()->createController(cd));
    EZ_ASSERT_DEV(m_pController != nullptr, "Failed to create character controller");

    PxRigidDynamic* pActor = m_pController->getActor();
    pActor->setMass(m_fMass);
    pActor->userData = cd.userData;

    PxShape* pShape = nullptr;
    pActor->getShapes(&pShape, 1);
    pShape->setSimulationFilterData(m_pData->m_FilterData);
    pShape->setQueryFilterData(m_pData->m_FilterData);
    pShape->userData = cd.userData;
  }
}

ezBitflags<ezPxCharacterShapeCollisionFlags> ezPxCharacterCapsuleShapeComponent::MoveShape(const ezVec3& vMoveDeltaGlobal)
{
  if (m_pController == nullptr)
    return ezPxCharacterShapeCollisionFlags::None;

  ezGameObject* pOwner = GetOwner();

  const float fElapsedTime = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();

  EZ_PX_WRITE_LOCK(*(m_pController->getScene()));

  // TODO: this call will crash, if the CC stands on an object that gets deleted during this frame
  // maybe have to do this update at some other time?
  PxControllerCollisionFlags collisionFlags = m_pController->move(ezPxConversionUtils::ToVec3(vMoveDeltaGlobal), 0.0f, fElapsedTime, m_pData->m_ControllerFilter);

  const ezVec3 vNewFootPos = ezPxConversionUtils::ToVec3(m_pController->getFootPosition());
  pOwner->SetGlobalPosition(vNewFootPos);

  return ezPxCharacterShapeCollisionFlags::FromPxFlags(collisionFlags);
}

bool ezPxCharacterCapsuleShapeComponent::TestShapeSweep(ezPhysicsCastResult& out_sweepResult, const ezVec3& vDirGlobal, float fDistance)
{
  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  ezTransform t;
  t.SetIdentity();
  t.m_vPosition = GetOwner()->GetGlobalPosition();
  t.m_vPosition.z += GetCurrentTotalHeight() * 0.5f;
  t.m_vPosition.z += 0.01f;

  ezPhysicsQueryParameters params(m_uiCollisionLayer, ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic, GetShapeId());

  return pModule->SweepTestCapsule(out_sweepResult, m_fCapsuleRadius, GetCurrentHeightValue(), t, vDirGlobal, fDistance, params);
}

bool ezPxCharacterCapsuleShapeComponent::TestShapeOverlap(const ezVec3& vGlobalFootPos, float fNewHeightValue)
{
  const ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  // little offset upwards, to get out of the ground, otherwise the CC is stuck in it
  ezVec3 vCenterPos = vGlobalFootPos;
  vCenterPos.z += GetCurrentTotalHeight() * 0.5f;
  vCenterPos.z += (fNewHeightValue - GetCurrentHeightValue()) * 0.5f;
  vCenterPos.z += 0.01f;

  fNewHeightValue += 0.005f;

  ezPhysicsQueryParameters params(m_uiCollisionLayer, ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic, GetShapeId());

  return pModule->OverlapTestCapsule(m_fCapsuleRadius, fNewHeightValue, ezTransform(vCenterPos), params);
}

float ezPxCharacterCapsuleShapeComponent::GetCurrentTotalHeight()
{
  return GetCurrentHeightValue() + m_fCapsuleRadius + m_fCapsuleRadius;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezPxCharacterProxyComponent_3_4 : public ezGraphPatch
{
public:
  ezPxCharacterProxyComponent_3_4()
    : ezGraphPatch("ezPxCharacterProxyComponent", 4)
  {
  }

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    context.RenameClass("ezPxCharacterCapsuleShapeComponent");
  }
};

ezPxCharacterProxyComponent_3_4 g_ezPxCharacterProxyComponent_3_4;

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Components_Implementation_PxCharacterProxyComponent);

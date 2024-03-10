#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxSimulatedRagdollComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <PxArticulationJoint.h>
#include <RendererCore/AnimationSystem/Skeleton.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxSimulatedRagdollComponent, 1, ezComponentMode::Dynamic)
{
  //EZ_BEGIN_PROPERTIES
  //{
  //}
  //EZ_END_PROPERTIES;
  //EZ_BEGIN_MESSAGEHANDLERS
  //{
  //  EZ_MESSAGE_HANDLER(ezMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
  //  EZ_MESSAGE_HANDLER(ezMsgAnimationPoseProposal, OnAnimationPoseProposal),
  //  EZ_MESSAGE_HANDLER(ezMsgPhysicsAddForce, AddForceAtPos),
  //  EZ_MESSAGE_HANDLER(ezMsgPhysicsAddImpulse, AddImpulseAtPos),
  //}
  //EZ_END_MESSAGEHANDLERS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxSimulatedRagdollComponent::ezPxSimulatedRagdollComponent() = default;
ezPxSimulatedRagdollComponent::~ezPxSimulatedRagdollComponent() = default;

void ezPxSimulatedRagdollComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
}

void ezPxSimulatedRagdollComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();
}

void ezPxSimulatedRagdollComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
}

void ezPxSimulatedRagdollComponent::OnDeactivated()
{
  SUPER::OnDeactivated();
}

void ezPxSimulatedRagdollComponent::WakeUp()
{
  m_pPxArticulation->wakeUp();
}

bool ezPxSimulatedRagdollComponent::IsSleeping() const
{
  return m_pPxArticulation->isSleeping();
}

void ezPxSimulatedRagdollComponent::ClearPhysicsObjects()
{
  if (m_pPxArticulation)
  {
    ezPhysXWorldModule* pPxModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
    EZ_PX_WRITE_LOCK(*pPxModule->GetPxScene());

    m_pPxArticulation->release();
    m_pPxArticulation = nullptr;
  }

  SUPER::ClearPhysicsObjects();
}

void ezPxSimulatedRagdollComponent::FinishSetupLimbs()
{
  m_pPxAggregate->addArticulation(*m_pPxArticulation);
}

void ezPxSimulatedRagdollComponent::SetupPxBasics(physx::PxPhysics* pPxApi, ezPhysXWorldModule* pPxModule)
{
  SUPER::SetupPxBasics(pPxApi, pPxModule);

  m_pPxArticulation = ezPhysX::GetSingleton()->GetPhysXAPI()->createArticulation();
  m_pPxArticulation->userData = m_pPxUserData;
}

void ezPxSimulatedRagdollComponent::CreateLimbBody(physx::PxPhysics* pPxApi, const LimbConfig& parentLimb, LimbConfig& thisLimb)
{
  physx::PxArticulationLink* pParentLink = (physx::PxArticulationLink*)parentLimb.m_pPxBody;
  physx::PxArticulationLink* pLink = m_pPxArticulation->createLink(pParentLink, ezPxConversionUtils::ToTransform(thisLimb.m_GlobalTransform));
  EZ_ASSERT_DEV(pLink != nullptr, "Ragdoll link creation failed. Too many bones? (max 64)");
  thisLimb.m_pPxBody = pLink;
}

void ezPxSimulatedRagdollComponent::CreateLimbJoint(physx::PxPhysics* pPxApi, const ezSkeletonJoint& thisJoint, physx::PxRigidBody* pPxParentBody, const ezTransform& parentFrame, physx::PxRigidBody* pPxThisBody, const ezTransform& thisFrame)
{
  PxArticulationLink* pLink = (PxArticulationLink*)pPxThisBody;
  PxArticulationJoint* pJoint = reinterpret_cast<PxArticulationJoint*>(pLink->getInboundJoint());

  // TODO ?
  pJoint->setInternalCompliance(0.7f);
  pJoint->setExternalCompliance(1.0f);

  pJoint->setParentPose(ezPxConversionUtils::ToTransform(parentFrame));
  pJoint->setChildPose(ezPxConversionUtils::ToTransform(thisFrame));

  if (thisJoint.GetHalfSwingLimitZ() >= ezAngle::MakeFromDegree(1) || thisJoint.GetHalfSwingLimitY() >= ezAngle::MakeFromDegree(1))
  {
    pJoint->setSwingLimitEnabled(true);
    pJoint->setSwingLimit(ezMath::Max(ezAngle::MakeFromDegree(1), thisJoint.GetHalfSwingLimitZ()).GetRadian(), ezMath::Max(ezAngle::MakeFromDegree(1), thisJoint.GetHalfSwingLimitY()).GetRadian());
    pJoint->setSwingLimitContactDistance(ezAngle::MakeFromDegree(10).GetRadian()); // ??
  }
  else
  {
    pJoint->setSwingLimitEnabled(false);
  }

  if (thisJoint.GetTwistLimitHalfAngle() > ezAngle::MakeFromDegree(0))
  {
    const ezAngle low = thisJoint.GetTwistLimitLow();
    const ezAngle high = thisJoint.GetTwistLimitHigh();

    pJoint->setTwistLimitEnabled(true);
    pJoint->setTwistLimit(low.GetRadian(), high.GetRadian());
    // pJoint->setTwistLimitContactDistance(); // ??
  }
  else
  {
    pJoint->setTwistLimitEnabled(false);
  }

  // TODO ?
  pJoint->setTangentialStiffness(1000.0f);
  pJoint->setTangentialDamping(25.0f);

  // TODO ?
  // pJoint->setDamping(25.0f);
}

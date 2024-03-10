#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxControlledRagdollComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PxConstraint.h>
#include <PxRigidDynamic.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <extensions/PxD6Joint.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxControlledRagdollComponent, 1, ezComponentMode::Dynamic)
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

ezPxControlledRagdollComponent::ezPxControlledRagdollComponent() = default;
ezPxControlledRagdollComponent::~ezPxControlledRagdollComponent() = default;

void ezPxControlledRagdollComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
}

void ezPxControlledRagdollComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();
}

void ezPxControlledRagdollComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
}

void ezPxControlledRagdollComponent::OnDeactivated()
{
  SUPER::OnDeactivated();
}

void ezPxControlledRagdollComponent::WakeUp()
{
  ((physx::PxRigidDynamic*)m_pPxRootBody)->wakeUp();
}

bool ezPxControlledRagdollComponent::IsSleeping() const
{
  // TODO: is this good enough ?
  return ((physx::PxRigidDynamic*)m_pPxRootBody)->isSleeping();
}

void ezPxControlledRagdollComponent::CreateLimbBody(physx::PxPhysics* pPxApi, const LimbConfig& parentLimb, LimbConfig& thisLimb)
{
  physx::PxRigidDynamic* pBody = pPxApi->createRigidDynamic(ezPxConversionUtils::ToTransform(thisLimb.m_GlobalTransform));
  thisLimb.m_pPxBody = pBody;

  pBody->setSolverIterationCounts(32, 16);
  pBody->setLinearDamping(1.0f);
  pBody->setAngularDamping(2.0f);
  pBody->setMaxAngularVelocity(50);
  pBody->setMaxLinearVelocity(1000);
}

void ezPxControlledRagdollComponent::CreateLimbJoint(physx::PxPhysics* pPxApi, const ezSkeletonJoint& thisJoint, physx::PxRigidBody* pPxParentBody, const ezTransform& parentFrame, physx::PxRigidBody* pPxThisBody, const ezTransform& thisFrame)
{
  physx::PxD6Joint* pJoint = PxD6JointCreate(*pPxApi, pPxParentBody, ezPxConversionUtils::ToTransform(parentFrame), pPxThisBody, ezPxConversionUtils::ToTransform(thisFrame));

  pJoint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
  pJoint->setProjectionLinearTolerance(0.01f);
  pJoint->setProjectionAngularTolerance(ezAngle::MakeFromDegree(1.0f).GetRadian());
  pJoint->setMotion(PxD6Axis::eX, PxD6Motion::eLOCKED);
  pJoint->setMotion(PxD6Axis::eY, PxD6Motion::eLOCKED);
  pJoint->setMotion(PxD6Axis::eZ, PxD6Motion::eLOCKED);

  if (thisJoint.GetHalfSwingLimitZ() >= ezAngle::MakeFromDegree(1) || thisJoint.GetHalfSwingLimitY() >= ezAngle::MakeFromDegree(1))
  {
    pJoint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
    pJoint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);

    const float stiffness = 1000;
    const float damping = 25000;

    const physx::PxSpring spring(stiffness, damping);

    const physx::PxJointLimitCone limit(
      ezMath::Max(ezAngle::MakeFromDegree(1), thisJoint.GetHalfSwingLimitY()).GetRadian(),
      ezMath::Max(ezAngle::MakeFromDegree(1), thisJoint.GetHalfSwingLimitZ()).GetRadian(),
      spring);

    pJoint->setSwingLimit(limit);
  }
  else
  {
    pJoint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
    pJoint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);
  }

  if (thisJoint.GetTwistLimitHalfAngle() >= ezAngle::MakeFromDegree(0))
  {
    pJoint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);

    const ezAngle low = thisJoint.GetTwistLimitLow();
    const ezAngle high = thisJoint.GetTwistLimitHigh();

    // const float stiffness = 1000;
    // const float damping = 0;

    // const physx::PxSpring spring(stiffness, damping);
    physx::PxJointAngularLimitPair limit(low.GetRadian(), high.GetRadian() /*, spring*/);
    limit.restitution = 0;
    limit.bounceThreshold = 1.0f;

    pJoint->setTwistLimit(limit);

    PxD6JointDrive d;
    d.forceLimit = 0.1f;
    d.stiffness = 0;
    d.damping = 0.7f;
    pJoint->setDrive(PxD6Drive::eSWING, d);
    pJoint->setDriveVelocity(PxVec3(PxZero), PxVec3(PxZero));
  }
  else
  {
    pJoint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLOCKED);
  }
}

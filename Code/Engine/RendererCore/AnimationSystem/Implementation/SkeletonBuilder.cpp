#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/SkeletonBuilder.h>

ezSkeletonBuilder::ezSkeletonBuilder() = default;
ezSkeletonBuilder::~ezSkeletonBuilder() = default;

ezUInt16 ezSkeletonBuilder::AddJoint(const char* szName, const ezTransform& localBindPose, ezUInt16 uiParentIndex /*= ezInvalidJointIndex*/)
{
  EZ_ASSERT_DEV(uiParentIndex == ezInvalidJointIndex || uiParentIndex < m_Joints.GetCount(), "Invalid parent index for joint");

  auto& joint = m_Joints.ExpandAndGetRef();

  joint.m_BindPoseLocal = localBindPose;
  joint.m_BindPoseGlobal = localBindPose;
  joint.m_sName.Assign(szName);
  joint.m_uiParentIndex = uiParentIndex;

  if (uiParentIndex != ezInvalidJointIndex)
  {
    joint.m_BindPoseGlobal = m_Joints[joint.m_uiParentIndex].m_BindPoseGlobal * joint.m_BindPoseLocal;
  }

  joint.m_InverseBindPoseGlobal = joint.m_BindPoseGlobal.GetInverse();

  return static_cast<ezUInt16>(m_Joints.GetCount() - 1);
}

void ezSkeletonBuilder::SetJointLimit(ezUInt16 uiJointIndex, const ezQuat& localOrientation, bool bLimitSwing, ezAngle halfSwingLimitY, ezAngle halfSwingLimitZ, bool bLimitTwist, ezAngle twistLimitHalfAngle, ezAngle twistLimitCenterAngle)
{
  auto& j = m_Joints[uiJointIndex];
  j.m_qLocalJointOrientation = localOrientation;
  j.m_HalfSwingLimitY = halfSwingLimitY;
  j.m_HalfSwingLimitZ = halfSwingLimitZ;
  j.m_TwistLimitHalfAngle = twistLimitHalfAngle;
  j.m_TwistLimitCenterAngle = twistLimitCenterAngle;
  j.m_bLimitSwing = bLimitSwing;
  j.m_bLimitTwist = bLimitTwist;
}

void ezSkeletonBuilder::BuildSkeleton(ezSkeleton& skeleton) const
{
  // EZ_ASSERT_DEV(HasJoints(), "Can't build a skeleton with no joints!");

  const ezUInt32 numJoints = m_Joints.GetCount();

  // Copy joints to skeleton
  skeleton.m_Joints.SetCount(numJoints);

  for (ezUInt32 i = 0; i < numJoints; ++i)
  {
    skeleton.m_Joints[i].m_sName = m_Joints[i].m_sName;
    skeleton.m_Joints[i].m_uiParentIndex = m_Joints[i].m_uiParentIndex;
    skeleton.m_Joints[i].m_BindPoseLocal = m_Joints[i].m_BindPoseLocal;

    skeleton.m_Joints[i].m_qLocalJointOrientation = m_Joints[i].m_qLocalJointOrientation;
    skeleton.m_Joints[i].m_HalfSwingLimitY = m_Joints[i].m_bLimitSwing ? m_Joints[i].m_HalfSwingLimitY : ezAngle();
    skeleton.m_Joints[i].m_HalfSwingLimitZ = m_Joints[i].m_bLimitSwing ? m_Joints[i].m_HalfSwingLimitZ : ezAngle();
    skeleton.m_Joints[i].m_TwistLimitHalfAngle = m_Joints[i].m_bLimitTwist ? m_Joints[i].m_TwistLimitHalfAngle : ezAngle();
    skeleton.m_Joints[i].m_TwistLimitCenterAngle = m_Joints[i].m_bLimitTwist ? m_Joints[i].m_TwistLimitCenterAngle : ezAngle();
  }
}

bool ezSkeletonBuilder::HasJoints() const
{
  return !m_Joints.IsEmpty();
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_SkeletonBuilder);

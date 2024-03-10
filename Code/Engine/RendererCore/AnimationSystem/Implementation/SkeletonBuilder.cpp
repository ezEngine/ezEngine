#include <RendererCore/RendererCorePCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/AnimationSystem/SkeletonBuilder.h>

ezSkeletonBuilder::ezSkeletonBuilder() = default;
ezSkeletonBuilder::~ezSkeletonBuilder() = default;

ezUInt16 ezSkeletonBuilder::AddJoint(ezStringView sName, const ezTransform& localRestPose, ezUInt16 uiParentIndex /*= ezInvalidJointIndex*/)
{
  EZ_ASSERT_DEV(uiParentIndex == ezInvalidJointIndex || uiParentIndex < m_Joints.GetCount(), "Invalid parent index for joint");

  auto& joint = m_Joints.ExpandAndGetRef();

  joint.m_RestPoseLocal = localRestPose;
  joint.m_RestPoseGlobal = localRestPose;
  joint.m_sName.Assign(sName);
  joint.m_uiParentIndex = uiParentIndex;

  if (uiParentIndex != ezInvalidJointIndex)
  {
    joint.m_RestPoseGlobal = m_Joints[joint.m_uiParentIndex].m_RestPoseGlobal * joint.m_RestPoseLocal;
  }

  joint.m_InverseRestPoseGlobal = joint.m_RestPoseGlobal.GetInverse();

  return static_cast<ezUInt16>(m_Joints.GetCount() - 1);
}

void ezSkeletonBuilder::SetJointLimit(ezUInt16 uiJointIndex, const ezQuat& qLocalOrientation, ezSkeletonJointType::Enum jointType, ezAngle halfSwingLimitY, ezAngle halfSwingLimitZ, ezAngle twistLimitHalfAngle, ezAngle twistLimitCenterAngle, float fStiffness)
{
  auto& j = m_Joints[uiJointIndex];
  j.m_qLocalJointOrientation = qLocalOrientation;
  j.m_JointType = jointType;
  j.m_HalfSwingLimitY = halfSwingLimitY;
  j.m_HalfSwingLimitZ = halfSwingLimitZ;
  j.m_TwistLimitHalfAngle = twistLimitHalfAngle;
  j.m_TwistLimitCenterAngle = twistLimitCenterAngle;
  j.m_fStiffness = fStiffness;
}


void ezSkeletonBuilder::SetJointSurface(ezUInt16 uiJointIndex, ezStringView sSurface)
{
  auto& j = m_Joints[uiJointIndex];
  j.m_sSurface = sSurface;
}

void ezSkeletonBuilder::SetJointCollisionLayer(ezUInt16 uiJointIndex, ezUInt8 uiCollsionLayer)
{
  auto& j = m_Joints[uiJointIndex];
  j.m_uiCollisionLayer = uiCollsionLayer;
}

void ezSkeletonBuilder::BuildSkeleton(ezSkeleton& ref_skeleton) const
{
  // EZ_ASSERT_DEV(HasJoints(), "Can't build a skeleton with no joints!");

  const ezUInt32 numJoints = m_Joints.GetCount();

  // Copy joints to skeleton
  ref_skeleton.m_Joints.SetCount(numJoints);

  for (ezUInt32 i = 0; i < numJoints; ++i)
  {
    ref_skeleton.m_Joints[i].m_sName = m_Joints[i].m_sName;
    ref_skeleton.m_Joints[i].m_uiParentIndex = m_Joints[i].m_uiParentIndex;
    ref_skeleton.m_Joints[i].m_RestPoseLocal = m_Joints[i].m_RestPoseLocal;

    ref_skeleton.m_Joints[i].m_JointType = m_Joints[i].m_JointType;
    ref_skeleton.m_Joints[i].m_qLocalJointOrientation = m_Joints[i].m_qLocalJointOrientation;
    ref_skeleton.m_Joints[i].m_HalfSwingLimitY = m_Joints[i].m_HalfSwingLimitY;
    ref_skeleton.m_Joints[i].m_HalfSwingLimitZ = m_Joints[i].m_HalfSwingLimitZ;
    ref_skeleton.m_Joints[i].m_TwistLimitHalfAngle = m_Joints[i].m_TwistLimitHalfAngle;
    ref_skeleton.m_Joints[i].m_TwistLimitCenterAngle = m_Joints[i].m_TwistLimitCenterAngle;

    ref_skeleton.m_Joints[i].m_uiCollisionLayer = m_Joints[i].m_uiCollisionLayer;
    ref_skeleton.m_Joints[i].m_hSurface = ezResourceManager::LoadResource<ezSurfaceResource>(m_Joints[i].m_sSurface);
    ref_skeleton.m_Joints[i].m_fStiffness = m_Joints[i].m_fStiffness;
  }
}

bool ezSkeletonBuilder::HasJoints() const
{
  return !m_Joints.IsEmpty();
}

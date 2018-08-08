#include <PCH.h>

#include <RendererCore/AnimationSystem/SkeletonBuilder.h>

ezSkeletonBuilder::ezSkeletonBuilder() = default;
ezSkeletonBuilder::~ezSkeletonBuilder() = default;

ezUInt32 ezSkeletonBuilder::AddJoint(const char* szName, const ezTransform& LocalTransform, ezUInt32 uiParentIndex /*= 0xFFFFFFFFu*/)
{
  EZ_ASSERT_DEV(uiParentIndex == 0xFFFFFFFFu || uiParentIndex < m_Joints.GetCount(), "Invalid parent index for joint");

  auto& joint = m_Joints.ExpandAndGetRef();

  joint.m_BindPoseLocal = LocalTransform;
  joint.m_BindPoseGlobal = LocalTransform;
  joint.m_sName.Assign(szName);
  joint.m_uiParentIndex = uiParentIndex;

  if (uiParentIndex != 0xFFFFFFFFu)
  {
    joint.m_BindPoseGlobal = m_Joints[joint.m_uiParentIndex].m_BindPoseGlobal * joint.m_BindPoseLocal;
  }

  joint.m_InverseBindPoseGlobal = joint.m_BindPoseGlobal.GetInverse();

  return m_Joints.GetCount() - 1;
}

void ezSkeletonBuilder::BuildSkeleton(ezSkeleton& skeleton) const
{
  EZ_ASSERT_DEV(HasJoints(), "Can't build a skeleton with no joints!");

  const ezUInt32 numJoints = m_Joints.GetCount();

  // Copy joints to skeleton
  skeleton.m_Joints.SetCountUninitialized(numJoints);

  for (ezUInt32 i = 0; i < numJoints; ++i)
  {
    skeleton.m_Joints[i].m_sName = m_Joints[i].m_sName;
    skeleton.m_Joints[i].m_uiParentIndex = m_Joints[i].m_uiParentIndex;
    skeleton.m_Joints[i].m_BindPoseLocal = m_Joints[i].m_BindPoseLocal;
    skeleton.m_Joints[i].m_InverseBindPoseGlobal = m_Joints[i].m_InverseBindPoseGlobal;
  }
}

bool ezSkeletonBuilder::HasJoints() const
{
  return !m_Joints.IsEmpty();
}

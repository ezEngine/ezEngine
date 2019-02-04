#include <PCH.h>

#include <RendererCore/AnimationSystem/Skeleton.h>

ezSkeleton::ezSkeleton() = default;
ezSkeleton::~ezSkeleton() = default;

ezUInt16 ezSkeleton::FindJointByName(const ezTempHashedString& sJointName) const
{
  for (ezUInt16 i = 0; i < m_Joints.GetCount(); ++i)
  {
    if (m_Joints[i].GetName() == sJointName)
    {
      return i;
    }
  }

  return ezInvalidJointIndex;
}

bool ezSkeleton::IsCompatibleWith(const ezSkeleton& other) const
{
  if (this == &other)
    return true;

  if (other.GetJointCount() != GetJointCount())
    return false;

  // TODO: This only checks the joint hierarchy, maybe it should check names or hierarchy based on names
  for (ezUInt32 i = 0; i < m_Joints.GetCount(); ++i)
  {
    if (other.m_Joints[i].GetParentIndex() != m_Joints[i].GetParentIndex())
    {
      return false;
    }
  }

  return true;
}

enum ezSkeletonVersion : ezUInt32
{
  Version1 = 1,
  Version2,

  ENUM_COUNT,
  Version_Current = ENUM_COUNT - 1
};

const ezUInt32 uiCurrentSkeletonVersion = 2;

void ezSkeleton::Save(ezStreamWriter& stream) const
{
  stream << (ezUInt32)ezSkeletonVersion::Version_Current;

  const ezUInt32 uiNumJoints = m_Joints.GetCount();
  stream << uiNumJoints;


  for (ezUInt32 i = 0; i < uiNumJoints; ++i)
  {
    stream << m_Joints[i].m_sName;
    stream << m_Joints[i].m_uiParentIndex;
    stream << m_Joints[i].m_BindPoseLocal;
    stream << m_Joints[i].m_InverseBindPoseGlobal;
  }
}

void ezSkeleton::Load(ezStreamReader& stream)
{
  m_Joints.Clear();

  ezUInt32 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= ezSkeletonVersion::Version_Current, "Skeleton versioning corrupt!");

  ezUInt32 uiNumJoints = 0;
  stream >> uiNumJoints;

  m_Joints.Reserve(uiNumJoints);

  if (uiVersion == 1)
  {
    for (ezUInt32 i = 0; i < uiNumJoints; ++i)
    {
      ezSkeletonJoint& joint = m_Joints.ExpandAndGetRef();

      ezUInt32 dummy;
      stream >> joint.m_sName;
      stream >> dummy;
      joint.m_uiParentIndex = dummy;
      stream >> joint.m_BindPoseLocal;
      stream >> joint.m_InverseBindPoseGlobal;
    }
  }
  else
  {
    for (ezUInt32 i = 0; i < uiNumJoints; ++i)
    {
      ezSkeletonJoint& joint = m_Joints.ExpandAndGetRef();

      stream >> joint.m_sName;
      stream >> joint.m_uiParentIndex;
      stream >> joint.m_BindPoseLocal;
      stream >> joint.m_InverseBindPoseGlobal;
    }
  }
}

bool ezSkeleton::IsJointDescendantOf(ezUInt16 uiJoint, ezUInt16 uiExpectedParent) const
{
  if (uiExpectedParent == ezInvalidJointIndex)
    return true;

  while (uiJoint != ezInvalidJointIndex)
  {
    if (uiJoint == uiExpectedParent)
      return true;

    uiJoint = m_Joints[uiJoint].m_uiParentIndex;
  }

  return false;
}

// void ezSkeleton::ApplyGlobalTransform(const ezMat3& transform)
//{
//  ezMat4 totalTransform(transform, ezVec3::ZeroVector());
//
//  const ezUInt32 uiNumJoints = m_Joints.GetCount();
//
//  for (ezUInt32 i = 0; i < uiNumJoints; ++i)
//  {
//    Joint& joint = m_Joints[i];
//
//    if (!joint.IsRootJoint())
//      continue;
//
//    joint.m_JointTransform = totalTransform * joint.m_JointTransform;
//  }
//}



EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_Skeleton);


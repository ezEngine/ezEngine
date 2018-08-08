#include <PCH.h>

#include <RendererCore/AnimationSystem/Skeleton.h>

ezSkeleton::ezSkeleton() = default;
ezSkeleton::~ezSkeleton() = default;

ezResult ezSkeleton::FindJointByName(const ezTempHashedString& sJointName, ezUInt32& out_uiIndex) const
{
  for (ezUInt32 i = 0; i < m_Joints.GetCount(); ++i)
  {
    if (m_Joints[i].GetName() == sJointName)
    {
      out_uiIndex = i;
      return EZ_SUCCESS;
    }
  }

  // Also fill joint index with bogus value to detect incorrect usage (no return value check) earlier.
  out_uiIndex = 0xFFFFFFFFu;
  return EZ_FAILURE;
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

  ENUM_COUNT,
  Version_Current = ENUM_COUNT - 1
};

const ezUInt32 uiCurrentSkeletonVersion = 1;

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

  for (ezUInt32 i = 0; i < uiNumJoints; ++i)
  {
    ezSkeletonJoint& joint = m_Joints.ExpandAndGetRef();

    stream >> joint.m_sName;
    stream >> joint.m_uiParentIndex;
    stream >> joint.m_BindPoseLocal;
    stream >> joint.m_InverseBindPoseGlobal;
  }
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

#include <RendererCorePCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>

ezSkeleton::ezSkeleton() = default;

ezSkeleton::ezSkeleton(ezSkeleton&& rhs)
{
  *this = std::move(rhs);
}

ezSkeleton::~ezSkeleton() = default;

void ezSkeleton::operator=(ezSkeleton&& rhs)
{
  m_Joints = std::move(rhs.m_Joints);
  m_pOzzSkeleton = std::move(rhs.m_pOzzSkeleton);
}

ezUInt16 ezSkeleton::FindJointByName(const ezTempHashedString& sJointName) const
{
  const ezUInt16 uiJointCount = static_cast<ezUInt16>(m_Joints.GetCount());
  for (ezUInt16 i = 0; i < uiJointCount; ++i)
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
  const ezUInt16 uiNumJoints = static_cast<ezUInt16>(m_Joints.GetCount());
  for (ezUInt32 i = 0; i < uiNumJoints; ++i)
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

static void BuildRawOzzSkeleton(const ezSkeleton& skeleton, ezUInt16 uiExpectedParent, ozz::animation::offline::RawSkeleton::Joint::Children& dstBones)
{
  ezHybridArray<ezUInt16, 6> children;

  for (ezUInt32 i = 0; i < skeleton.GetJointCount(); ++i)
  {
    if (skeleton.GetJointByIndex(i).GetParentIndex() == uiExpectedParent)
    {
      children.PushBack(i);
    }
  }

  dstBones.resize((size_t)children.GetCount());

  for (ezUInt32 i = 0; i < children.GetCount(); ++i)
  {
    const auto& srcJoint = skeleton.GetJointByIndex(children[i]);
    const auto& srcTransform = srcJoint.GetBindPoseLocalTransform();
    auto& dstJoint = dstBones[i];

    dstJoint.name = srcJoint.GetName().GetData();

    dstJoint.transform.translation.x = srcTransform.m_vPosition.x;
    dstJoint.transform.translation.y = srcTransform.m_vPosition.y;
    dstJoint.transform.translation.z = srcTransform.m_vPosition.z;
    dstJoint.transform.rotation.x = srcTransform.m_qRotation.v.x;
    dstJoint.transform.rotation.y = srcTransform.m_qRotation.v.y;
    dstJoint.transform.rotation.z = srcTransform.m_qRotation.v.z;
    dstJoint.transform.rotation.w = srcTransform.m_qRotation.w;
    dstJoint.transform.scale.x = srcTransform.m_vScale.x;
    dstJoint.transform.scale.y = srcTransform.m_vScale.y;
    dstJoint.transform.scale.z = srcTransform.m_vScale.z;

    BuildRawOzzSkeleton(skeleton, children[i], dstJoint.children);
  }
}

const ozz::animation::Skeleton& ezSkeleton::GetOzzSkeleton() const
{
  if (m_pOzzSkeleton)
    return *m_pOzzSkeleton.Borrow();

  ozz::animation::offline::RawSkeleton rawSkeleton;
  BuildRawOzzSkeleton(*this, ezInvalidJointIndex, rawSkeleton.roots);

  ozz::animation::offline::SkeletonBuilder skeletonBuilder;
  const auto pOzzSkeleton = skeletonBuilder(rawSkeleton);

  m_pOzzSkeleton = EZ_DEFAULT_NEW(ozz::animation::Skeleton);

  ezOzzUtils::CopySkeleton(m_pOzzSkeleton.Borrow(), pOzzSkeleton.get());

  return *m_pOzzSkeleton.Borrow();
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

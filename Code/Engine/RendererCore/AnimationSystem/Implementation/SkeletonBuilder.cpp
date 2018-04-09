
#include <RendererCore/PCH.h>
#include <RendererCore/AnimationSystem/SkeletonBuilder.h>

ezSkeletonBuilder::ezSkeletonBuilder()
  : m_eSkinningMode(ezSkeleton::Mode::FourBones)
{
}

ezUInt32 ezSkeletonBuilder::AddBone(const char* szName, const ezMat4& LocalTransform, ezUInt32 uiParentIndex /*= 0xFFFFFFFFu*/)
{
  auto& bone = m_Bones.ExpandAndGetRef();

  bone.m_BoneTransform = LocalTransform;
  bone.m_InverseBindPoseTransform = LocalTransform.GetInverse();
  bone.m_sName.Assign(szName);
  bone.m_uiParentIndex = uiParentIndex;

  return m_Bones.GetCount() - 1;
}

void ezSkeletonBuilder::SetSkinningMode(ezSkeleton::Mode eSkinningMode)
{
  m_eSkinningMode = eSkinningMode;
}

ezUniquePtr<ezSkeleton> ezSkeletonBuilder::CreateSkeletonInstance() const
{
  EZ_ASSERT_DEV(m_Bones.GetCount() > 0, "Can't create a skeleton instance with no bones!");

  ezUniquePtr<ezSkeleton> pSkeleton = EZ_DEFAULT_NEW(ezSkeleton);

  pSkeleton->m_eSkinningMode = m_eSkinningMode;

  // Copy bones to skeleton
  pSkeleton->m_Bones = m_Bones;

  // TODO: Compatibility hash here?

  return pSkeleton;
}

bool ezSkeletonBuilder::HasBones() const
{
  return !m_Bones.IsEmpty();
}

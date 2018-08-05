#include <PCH.h>

#include <RendererCore/AnimationSystem/SkeletonBuilder.h>

ezSkeletonBuilder::ezSkeletonBuilder()
    : m_eSkinningMode(ezSkeleton::Mode::FourBones)
{
}

ezUInt32 ezSkeletonBuilder::AddBone(const char* szName, const ezMat4& LocalTransform, ezUInt32 uiParentIndex /*= 0xFFFFFFFFu*/)
{
  auto& bone = m_Bones.ExpandAndGetRef();

  bone.m_BindPoseLocal = LocalTransform;
  bone.m_BindPoseGlobal = LocalTransform;
  bone.m_sName.Assign(szName);
  bone.m_uiParentIndex = uiParentIndex;

  if (!bone.IsRootBone())
  {
    bone.m_BindPoseGlobal = m_Bones[bone.m_uiParentIndex].m_BindPoseGlobal * bone.m_BindPoseLocal;
  }

  bone.m_InverseBindPoseGlobal = bone.m_BindPoseGlobal.GetInverse();

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

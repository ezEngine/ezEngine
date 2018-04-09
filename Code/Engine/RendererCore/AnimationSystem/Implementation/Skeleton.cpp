
#include <RendererCore/PCH.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <Foundation/Reflection/Reflection.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkeleton, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE


ezSkeleton::ezSkeleton()
  : m_bAnyPoseCreated(false)
  , m_eSkinningMode(Mode::FourBones)
{
}


ezSkeleton::ezSkeleton(const ezSkeleton& Other)
  : m_bAnyPoseCreated(false)
  , m_eSkinningMode(Other.m_eSkinningMode)
{
  m_Bones = Other.m_Bones;
}

void ezSkeleton::operator = (const ezSkeleton& Other)
{
  m_bAnyPoseCreated = false;
  m_pBindSpacePose.Reset();
  m_pObjectSpaceBindPose.Reset();

  m_Bones = Other.m_Bones;
  m_eSkinningMode = Other.m_eSkinningMode;
}

ezSkeleton::~ezSkeleton()
{
  m_pBindSpacePose.Reset();
  m_pObjectSpaceBindPose.Reset();
  m_Bones.Clear();
}

ezUInt32 ezSkeleton::GetBoneCount() const
{
  return m_Bones.GetCount();
}

bool ezSkeleton::FindBoneByName(const char* szName, ezUInt32& uiBoneIndex) const
{
  ezTempHashedString NameString(szName);

  for (ezUInt32 i = 0; i < m_Bones.GetCount(); ++i)
  {
    if (m_Bones[i].GetName() == NameString)
    {
      uiBoneIndex = i;
      return true;
    }
  }

  // Also fill bone index with bogus value to detect incorrect usage (no return value check) earlier.
  uiBoneIndex = 0xFFFFFFFFu;
  return false;
}

ezUniquePtr<ezAnimationPose> ezSkeleton::CreatePose() const
{
  // Track the fact that we created a pose, thus the skeleton can't be modified anymore.
  // (Just for wrong API usage detection)
  m_bAnyPoseCreated = true;

  return EZ_DEFAULT_NEW(ezAnimationPose, this);
}

void ezSkeleton::SetAnimationPoseToBindPose(ezAnimationPose* pPose) const
{
  EZ_ASSERT_DEV(pPose, "Invalid pose pointer!");
  EZ_ASSERT_DEV(pPose->GetBoneTransformCount() == GetBoneCount(), "Pose and skeleton have different bone count!");

  // TODO: Check additional compatibility of pose object with this skeleton?

  // Copy bind pose to pose by using the initial bone transforms of the skeleton.
  for (ezUInt32 i = 0; i < m_Bones.GetCount(); ++i)
  {
    pPose->SetBoneTransform(i, m_Bones[i].GetBoneTransform());
  }
}

void ezSkeleton::CalculateObjectSpaceAnimationPoseMatrices(ezAnimationPose* pPose) const
{
  EZ_ASSERT_DEV(pPose, "Invalid pose pointer!");
  EZ_ASSERT_DEV(pPose->GetBoneTransformCount() == GetBoneCount(), "Pose and skeleton have different bone count!");

  // TODO: Check additional compatibility of pose object with this skeleton?

  // Since the bones are sorted (at least no child bone comes before it's parent bone)
  // we can simply grab the already stored parent transform from the pose to get the multiplied
  // transforms up to the child bone we currently work on.
  for (ezUInt32 i = 0; i < m_Bones.GetCount(); ++i)
  {
    // If it is a root bone the transform is already final.
    if (m_Bones[i].IsRootBone())
    {
      pPose->SetBoneTransform(i, m_Bones[i].GetInverseBindPoseTransform() * m_Bones[i].GetBoneTransform());
    }
    // If not: grab transform of parent bone and use it to make the final transform for this bone
    else
    {
      // TODO: Verify that multiplication order is correct
      pPose->SetBoneTransform(i, m_Bones[i].GetInverseBindPoseTransform() * m_Bones[i].GetBoneTransform() * pPose->GetBoneTransform(m_Bones[i].GetParentIndex()));
    }
  }
}

bool ezSkeleton::IsCompatibleWith(const ezSkeleton* pOtherSkeleton) const
{
  if (this == pOtherSkeleton)
    return true;

  if (!pOtherSkeleton)
    return false;

  if (pOtherSkeleton->GetBoneCount() != GetBoneCount())
    return false;

  // TODO: This only checks the bone hierarchy, maybe it should check names or hierarchy based on names
  for (ezUInt32 i = 0; i < m_Bones.GetCount(); ++i)
  {
    if (pOtherSkeleton->m_Bones[i].GetParentIndex() != m_Bones[i].GetParentIndex())
    {
      return false;
    }
  }

  return true;
}

const ezUInt32 uiCurrentSkeletonVersion = 1;

void ezSkeleton::Save(ezStreamWriter& stream) const
{
  stream << uiCurrentSkeletonVersion;

  stream << ezUInt32(m_eSkinningMode);

  const ezUInt32 uiNumBones = m_Bones.GetCount();

  stream << uiNumBones;

  for (ezUInt32 i = 0; i < uiNumBones; ++i)
  {
    stream << m_Bones[i].m_sName;
    stream << m_Bones[i].m_uiParentIndex;
    stream << m_Bones[i].m_BoneTransform;
    stream << m_Bones[i].m_InverseBindPoseTransform;
  }
}

ezResult ezSkeleton::Load(ezStreamReader& stream)
{
  m_Bones.Clear();

  ezUInt32 uiVersion = 0;
  stream >> uiVersion;

  if (uiVersion != uiCurrentSkeletonVersion)
  {
    ezLog::SeriousWarning("Skeleton versioning corrupt!");
    return EZ_FAILURE;
  }

  ezUInt32 uiSkinningMode = 0;
  stream >> uiSkinningMode;

  switch (uiSkinningMode)
  {
  case ezUInt32(Mode::SingleBone):
    m_eSkinningMode = Mode::SingleBone;
    break;
  case ezUInt32(Mode::FourBones):
    m_eSkinningMode = Mode::FourBones;
    break;
  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  ezUInt32 uiNumBones = 0;

  stream >> uiNumBones;

  m_Bones.Reserve(uiNumBones);

  for (ezUInt32 i = 0; i < uiNumBones; ++i)
  {
    Bone& bone = m_Bones.ExpandAndGetRef();

    stream >> bone.m_sName;
    stream >> bone.m_uiParentIndex;
    stream >> bone.m_BoneTransform;
    stream >> bone.m_InverseBindPoseTransform;
  }

  return EZ_SUCCESS;
}


void ezSkeleton::ApplyGlobalTransform(const ezMat3& transform)
{
  ezMat4 totalTransform(transform, ezVec3::ZeroVector());

  const ezUInt32 uiNumBones = m_Bones.GetCount();

  for (ezUInt32 i = 0; i < uiNumBones; ++i)
  {
    Bone& bone = m_Bones[i];

    if (!bone.IsRootBone())
      continue;

    bone.m_BoneTransform = totalTransform * bone.m_BoneTransform;
  }
}

const ezAnimationPose* ezSkeleton::GetBindSpacePose() const
{
  if (!m_pBindSpacePose)
  {
    m_pBindSpacePose = CreatePose();
    SetAnimationPoseToBindPose(m_pBindSpacePose.Borrow());
  }

  return m_pBindSpacePose.Borrow();
}

const ezAnimationPose* ezSkeleton::GetBindSpacePoseInObjectSpace() const
{
  if (!m_pObjectSpaceBindPose)
  {
    m_pObjectSpaceBindPose = CreatePose();
    SetAnimationPoseToBindPose(m_pObjectSpaceBindPose.Borrow());
    CalculateObjectSpaceAnimationPoseMatrices(m_pObjectSpaceBindPose.Borrow());
  }

  return m_pObjectSpaceBindPose.Borrow();
}


ezSkeleton::Mode ezSkeleton::GetSkinningMode() const
{
  return m_eSkinningMode;
}

#include <PCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/SimpleAnimationComponent.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSimpleAnimationComponent, 4, ezComponentMode::Dynamic);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("AnimationClip", GetAnimationClipFile, SetAnimationClipFile)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
    EZ_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new ezAssetBrowserAttribute("Skeleton")),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSimpleAnimationComponent::ezSimpleAnimationComponent() {}

ezSimpleAnimationComponent::~ezSimpleAnimationComponent() {}

void ezSimpleAnimationComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  ezAngle m_DegreePerSecond;
  s << m_DegreePerSecond;
  s << m_hAnimationClip;
  s << m_hSkeleton;
}

void ezSimpleAnimationComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  ezAngle m_DegreePerSecond;
  s >> m_DegreePerSecond;
  s >> m_hAnimationClip;

  if (uiVersion >= 4)
  {
    s >> m_hSkeleton;
  }
}

void ezSimpleAnimationComponent::OnActivated()
{
  SUPER::OnActivated();

  // make sure the skinning buffer is deleted
  EZ_ASSERT_DEBUG(m_hSkinningTransformsBuffer.IsInvalidated(), "The skinning buffer should not exist at this time");

  if (m_hSkeleton.IsValid())
  {
    ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::NoFallback);

    const ezSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
    m_pAnimationPose = skeleton.CreatePose();
    skeleton.SetAnimationPoseToBindPose(m_pAnimationPose.Borrow());
    skeleton.CalculateObjectSpaceAnimationPoseMatrices(m_pAnimationPose.Borrow());

    m_SkinningMatrices = m_pAnimationPose->GetAllBoneTransforms();

    // for (ezUInt32 i = 0; i < m_BoneMatrices.GetCount(); ++i)
    //{
    //  m_BoneMatrices[i].SetIdentity();
    //}

    // Create the buffer for the skinning matrices
    ezGALBufferCreationDescription BufferDesc;
    BufferDesc.m_uiStructSize = sizeof(ezMat4);
    BufferDesc.m_uiTotalSize = BufferDesc.m_uiStructSize * m_SkinningMatrices.GetCount();
    BufferDesc.m_bUseAsStructuredBuffer = true;
    BufferDesc.m_bAllowShaderResourceView = true;
    BufferDesc.m_ResourceAccess.m_bImmutable = false;

    m_hSkinningTransformsBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(
        BufferDesc, ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(m_SkinningMatrices.GetPtr()), BufferDesc.m_uiTotalSize));
  }
}

void ezSimpleAnimationComponent::SetAnimationClip(const ezAnimationClipResourceHandle& hResource)
{
  m_hAnimationClip = hResource;
}

void ezSimpleAnimationComponent::SetAnimationClipFile(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  SetAnimationClip(hResource);
}

const char* ezSimpleAnimationComponent::GetAnimationClipFile() const
{
  if (!m_hAnimationClip.IsValid())
    return "";

  return m_hAnimationClip.GetResourceID();
}


void ezSimpleAnimationComponent::SetSkeleton(const ezSkeletonResourceHandle& hResource)
{
  m_hSkeleton = hResource;
}

void ezSimpleAnimationComponent::SetSkeletonFile(const char* szFile)
{
  ezSkeletonResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezSkeletonResource>(szFile);
  }

  SetSkeleton(hResource);
}

const char* ezSimpleAnimationComponent::GetSkeletonFile() const
{
  if (!m_hSkeleton.IsValid())
    return "";

  return m_hSkeleton.GetResourceID();
}



void ezSimpleAnimationComponent::Update()
{
  if (!m_hAnimationClip.IsValid())
    return;

  ezResourceLock<ezAnimationClipResource> pAnimClip(m_hAnimationClip);
  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton);
  const ezSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  const ezTime animDuration = pAnimClip->GetDescriptor().GetDuration();

  if (animDuration < ezTime::Zero())
    return;

  m_AnimationTime += GetWorld()->GetClock().GetTimeDiff();

  // loop the animation
  while (m_AnimationTime >= animDuration)
  {
    m_AnimationTime -= animDuration;
  }

  // TODO: reasons why stuff may not work
  // The skeleton is rotated differently than the mesh, it seems not possible to align them correctly (forward dir)
  // animation clip data is neither scaled nor rotated
  // that whole (inverse) bindpose stuff is complicated

  // dummy stuff that doesn't work very well
  {
    skeleton.SetAnimationPoseToBindPose(m_pAnimationPose.Borrow());

    ezUInt32 uiHeadBone;
    if (skeleton.FindBoneByName("Bip01_L_Calf", uiHeadBone))
    {
      ezMat4 mRot;
      mRot.SetRotationMatrixX(ezAngle::Degree((float)m_AnimationTime.GetSeconds() * 30.0f));

      ezMat4 mCur = m_pAnimationPose->GetBoneTransform(uiHeadBone);
      // m_pAnimationPose->SetBoneTransform(uiHeadBone, mCur * mRot * mCur.GetInverse());
      m_pAnimationPose->SetBoneTransform(uiHeadBone, mCur * mRot);

      ezMat4 mTrans;
      mTrans.SetIdentity();
      mTrans.SetTranslationMatrix(ezVec3(0, 0, (float)m_AnimationTime.GetSeconds()));
      // m_pAnimationPose->SetBoneTransform(uiHeadBone, mTrans);
    }
  }

  // using real animation data (works even less)
  if (false)
  {
    const auto& animDesc = pAnimClip->GetDescriptor();

    double fAnimLerp = 0;
    const ezUInt32 uiFirstFrame = animDesc.GetFrameAt(m_AnimationTime, fAnimLerp);

    const auto& animatedBones = animDesc.GetAllBoneIndices();
    for (ezUInt32 b = 0; b < animatedBones.GetCount(); ++b)
    {
      const ezHashedString sBoneName = animatedBones.GetKey(b);

      ezUInt32 uiHeadBone;
      if (skeleton.FindBoneByName(sBoneName, uiHeadBone))
      {
        const ezMat4 boneMat = animDesc.GetBoneKeyframes(b)[uiFirstFrame];

        // TODO: animations are not scaled yet, these matrices do not include the rotation and down-scale that the skeleton already has

        m_pAnimationPose->SetBoneTransform(uiHeadBone, boneMat);
      }
    }
  }

  skeleton.CalculateObjectSpaceAnimationPoseMatrices(m_pAnimationPose.Borrow());
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_SimpleAnimationComponent);

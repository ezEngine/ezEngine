#include <PCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/AnimatedMeshComponent.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAnimatedMeshComponent, 10, ezComponentMode::Dynamic);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Animated Mesh")),
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new ezAssetBrowserAttribute("Material")),

    EZ_ACCESSOR_PROPERTY("AnimationClip", GetAnimationClipFile, SetAnimationClipFile)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
    EZ_ACCESSOR_PROPERTY("Loop", GetLoopAnimation, SetLoopAnimation),
    EZ_ACCESSOR_PROPERTY("Speed", GetAnimationSpeed, SetAnimationSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("ApplyRootMotion", m_bApplyRootMotion),
    EZ_MEMBER_PROPERTY("VisualizeSkeleton", m_bVisualizeSkeleton),
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

ezAnimatedMeshComponent::ezAnimatedMeshComponent() {}

ezAnimatedMeshComponent::~ezAnimatedMeshComponent() {}

void ezAnimatedMeshComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_bApplyRootMotion;
  m_AnimationClipSampler.Save(s);

  s << m_bVisualizeSkeleton;
}

void ezAnimatedMeshComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  EZ_ASSERT_DEV(uiVersion >= 9, "Unsupported version, delete the file and reexport it");

  s >> m_bApplyRootMotion;
  m_AnimationClipSampler.Load(s);

  s >> m_bVisualizeSkeleton;
}

void ezAnimatedMeshComponent::OnSimulationStarted()
{
  SUPER::OnActivated();

  // make sure the skinning buffer is deleted
  EZ_ASSERT_DEBUG(m_hSkinningTransformsBuffer.IsInvalidated(), "The skinning buffer should not exist at this time");

  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::NoFallback);
    m_hSkeleton = pMesh->GetSkeleton();
  }

  if (m_hSkeleton.IsValid())
  {
    ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::NoFallback);

    const ezSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
    m_AnimationPose.Configure(skeleton);
    m_AnimationPose.ConvertFromLocalSpaceToObjectSpace(skeleton);
    m_AnimationPose.ConvertFromObjectSpaceToSkinningSpace(skeleton);

    // m_SkinningMatrices = m_AnimationPose.GetAllTransforms();

    // Create the buffer for the skinning matrices
    ezGALBufferCreationDescription BufferDesc;
    BufferDesc.m_uiStructSize = sizeof(ezMat4);
    BufferDesc.m_uiTotalSize = BufferDesc.m_uiStructSize * m_AnimationPose.GetTransformCount();
    BufferDesc.m_bUseAsStructuredBuffer = true;
    BufferDesc.m_bAllowShaderResourceView = true;
    BufferDesc.m_ResourceAccess.m_bImmutable = false;

    m_hSkinningTransformsBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(
        BufferDesc,
        ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(m_AnimationPose.GetAllTransforms().GetPtr()), BufferDesc.m_uiTotalSize));
  }

  m_AnimationClipSampler.RestartAnimation();
}

void ezAnimatedMeshComponent::SetAnimationClip(const ezAnimationClipResourceHandle& hResource)
{
  m_AnimationClipSampler.SetAnimationClip(hResource);
}

const ezAnimationClipResourceHandle& ezAnimatedMeshComponent::GetAnimationClip() const
{
  return m_AnimationClipSampler.GetAnimationClip();
}

void ezAnimatedMeshComponent::SetAnimationClipFile(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  SetAnimationClip(hResource);
}

const char* ezAnimatedMeshComponent::GetAnimationClipFile() const
{
  if (!m_AnimationClipSampler.GetAnimationClip().IsValid())
    return "";

  return m_AnimationClipSampler.GetAnimationClip().GetResourceID();
}

bool ezAnimatedMeshComponent::GetLoopAnimation() const
{
  return m_AnimationClipSampler.GetLooping();
}

void ezAnimatedMeshComponent::SetLoopAnimation(bool loop)
{
  m_AnimationClipSampler.SetLooping(loop);
}


float ezAnimatedMeshComponent::GetAnimationSpeed() const
{
  return m_AnimationClipSampler.GetPlaybackSpeed();
}

void ezAnimatedMeshComponent::SetAnimationSpeed(float speed)
{
  m_AnimationClipSampler.SetPlaybackSpeed(speed);
}

void ezAnimatedMeshComponent::Update()
{
  if (!m_AnimationClipSampler.GetAnimationClip().IsValid() || !m_hSkeleton.IsValid())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton);
  const ezSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  ezTransform rootMotion;
  rootMotion.SetIdentity();

  m_AnimationPose.SetToBindPoseInLocalSpace(skeleton);
  m_AnimationClipSampler.Step(GetWorld()->GetClock().GetTimeDiff());
  m_AnimationClipSampler.Execute(skeleton, m_AnimationPose, &rootMotion);

  m_AnimationPose.ConvertFromLocalSpaceToObjectSpace(skeleton);

  if (m_bVisualizeSkeleton)
  {
    m_AnimationPose.VisualizePose(GetWorld(), skeleton, GetOwner()->GetGlobalTransform());
  }

  m_AnimationPose.ConvertFromObjectSpaceToSkinningSpace(skeleton);

  ezArrayPtr<ezMat4> pRenderMatrices = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezMat4, m_AnimationPose.GetTransformCount());
  ezMemoryUtils::Copy(pRenderMatrices.GetPtr(), m_AnimationPose.GetAllTransforms().GetPtr(), m_AnimationPose.GetTransformCount());

  m_SkinningMatrices = pRenderMatrices;

  if (m_bApplyRootMotion)
  {
    auto* pOwner = GetOwner();

    const ezQuat qOldRot = pOwner->GetLocalRotation();
    const ezVec3 vNewPos = qOldRot * (rootMotion.m_vPosition * pOwner->GetGlobalScaling().x) + pOwner->GetLocalPosition();
    const ezQuat qNewRot = rootMotion.m_qRotation * qOldRot;

    pOwner->SetLocalPosition(vNewPos);
    pOwner->SetLocalRotation(qNewRot);
  }
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezAnimatedMeshComponentPatch_4_5 : public ezGraphPatch
{
public:
  ezAnimatedMeshComponentPatch_4_5()
      : ezGraphPatch("ezSimpleAnimationComponent", 5)
  {
  }
  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    context.RenameClass("ezAnimatedMeshComponent");
  }
};

ezAnimatedMeshComponentPatch_4_5 g_ezAnimatedMeshComponentPatch_4_5;

#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererFoundation/Device/Device.h>

#include <ozz/animation/runtime/skeleton.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAnimatedMeshComponent, 13, ezComponentMode::Dynamic); // TODO: why dynamic ?
{
  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgAnimationPoseUpdated, OnAnimationPoseUpdated)
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezAnimatedMeshComponent::ezAnimatedMeshComponent() = default;
ezAnimatedMeshComponent::~ezAnimatedMeshComponent() = default;

void ezAnimatedMeshComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();
}

void ezAnimatedMeshComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  EZ_ASSERT_DEV(uiVersion >= 13, "Unsupported version, delete the file and reexport it");
}

void ezAnimatedMeshComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // make sure the skinning buffer is deleted
  EZ_ASSERT_DEBUG(m_hSkinningTransformsBuffer.IsInvalidated(), "The skinning buffer should not exist at this time");

  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::BlockTillLoaded);
    m_hSkeleton = pMesh->GetSkeleton();
  }

  if (m_hSkeleton.IsValid())
  {
    ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);

    const ezSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
    m_AnimationPose.Configure(skeleton);
    m_AnimationPose.ConvertFromLocalSpaceToObjectSpace(skeleton);

    m_AnimationPose.ConvertFromObjectSpaceToSkinningSpace(skeleton);


    // Create the buffer for the skinning matrices
    ezGALBufferCreationDescription BufferDesc;
    BufferDesc.m_uiStructSize = sizeof(ezMat4);
    BufferDesc.m_uiTotalSize = BufferDesc.m_uiStructSize * m_AnimationPose.GetTransformCount();
    BufferDesc.m_bUseAsStructuredBuffer = true;
    BufferDesc.m_bAllowShaderResourceView = true;
    BufferDesc.m_ResourceAccess.m_bImmutable = false;

    m_hSkinningTransformsBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(BufferDesc, ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(m_AnimationPose.GetAllTransforms().GetPtr()), BufferDesc.m_uiTotalSize));
  }
}

ezMeshRenderData* ezAnimatedMeshComponent::CreateRenderData() const
{
  // this copy is necessary for the multi-threaded renderer to not access m_AnimationPose while we update it
  ezArrayPtr<ezMat4> pRenderMatrices = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezMat4, m_AnimationPose.GetTransformCount());
  ezMemoryUtils::Copy(pRenderMatrices.GetPtr(), m_AnimationPose.GetAllTransforms().GetPtr(), m_AnimationPose.GetTransformCount());

  m_SkinningMatrices = pRenderMatrices;


  return SUPER::CreateRenderData();
}

void ezAnimatedMeshComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg)
{
  if (!m_hSkeleton.IsValid())
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  if (&pSkeleton->GetDescriptor().m_Skeleton != msg.m_pSkeleton)
  {
    // TODO: allow compatible skeletons?
    return;
  }

  const ezSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;

  for (ezUInt32 b = 0; b < skeleton.GetJointCount(); ++b)
  {
    m_AnimationPose.SetTransform(b, msg.m_ModelTransforms[b]);
  }

  m_AnimationPose.ConvertFromObjectSpaceToSkinningSpace(skeleton);
  m_SkinningMatrices = m_AnimationPose.GetAllTransforms();
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AnimatedMeshComponent);

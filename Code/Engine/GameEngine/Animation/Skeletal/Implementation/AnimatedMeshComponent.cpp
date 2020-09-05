#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererFoundation/Device/Device.h>

#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAnimatedMeshComponent, 13, ezComponentMode::Dynamic); // TODO: why dynamic ?
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("VisualizeBindPose", m_bVisualizeBindPose)
  } 
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    EZ_MESSAGE_HANDLER(ezMsgQueryAnimationSkeleton, OnQueryAnimationSkeleton),
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

  if (!m_hMesh.IsValid())
    return;

  ezMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  m_hSkeleton = msg.m_hSkeleton;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::BlockTillLoaded);

  if (!m_hSkeleton.IsValid())
  {
    // if we didn't get a skeleton from another component, via the message, try to get one from the mesh
    m_hSkeleton = pMesh->m_hDefaultSkeleton;
  }

  if (!m_hSkeleton.IsValid())
    return;

  m_AnimTransforms.SetCountUninitialized(pMesh->m_Bones.GetCount());

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded);

  const ezSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
  ezAnimationPose m_AnimationPose;
  m_AnimationPose.Configure(skeleton);
  m_AnimationPose.ConvertFromLocalSpaceToObjectSpace(skeleton);

  m_Lines.Clear();

  for (auto itBone : pMesh->m_Bones)
  {
    const ezUInt16 uiJointIdx = skeleton.FindJointByName(ezTempHashedString(itBone.Key().GetData()));
    const ezMat4 modelSpaceTransform = m_AnimationPose.GetTransform(uiJointIdx);

    m_AnimTransforms[itBone.Value().m_uiBoneIndex] = modelSpaceTransform * itBone.Value().m_GlobalInverseBindPoseMatrix;

    ezUInt16 uiParentJointIdx = skeleton.GetJointByIndex(uiJointIdx).GetParentIndex();
    while (uiParentJointIdx != ezInvalidJointIndex)
    {
      auto parentName = skeleton.GetJointByIndex(uiParentJointIdx).GetName();

      auto itParent = pMesh->m_Bones.Find(parentName);

      if (itParent.IsValid())
      {
        auto& l = m_Lines.ExpandAndGetRef();
        l.m_start = itBone.Value().m_GlobalInverseBindPoseMatrix.GetInverse().GetTranslationVector();
        l.m_end = itParent.Value().m_GlobalInverseBindPoseMatrix.GetInverse().GetTranslationVector();
        break;
      }

      uiParentJointIdx = skeleton.GetJointByIndex(uiParentJointIdx).GetParentIndex();
    }
  }

  // m_AnimTransforms.Clear();
  // m_AnimationPose.ConvertFromObjectSpaceToSkinningSpace(skeleton);

  // Create the buffer for the skinning matrices
  CreateSkinningTransformBuffer(m_AnimTransforms);
}

ezMeshRenderData* ezAnimatedMeshComponent::CreateRenderData() const
{
  // this copy is necessary for the multi-threaded renderer to not access m_AnimationPose while we update it
  ezArrayPtr<ezMat4> pRenderMatrices = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezMat4, m_AnimTransforms.GetCount());
  ezMemoryUtils::Copy(pRenderMatrices.GetPtr(), m_AnimTransforms.GetData(), m_AnimTransforms.GetCount());

  m_SkinningMatrices = pRenderMatrices;

  if (m_bVisualizeBindPose && !m_Lines.IsEmpty())
  {
    ezDebugRenderer::DrawLines(GetWorld(), m_Lines, ezColor::GreenYellow, GetOwner()->GetGlobalTransform());
  }

  return SUPER::CreateRenderData();
}

void ezAnimatedMeshComponent::OnAnimationPoseUpdated(ezMsgAnimationPoseUpdated& msg)
{
  if (!m_hSkeleton.IsValid() || !m_hMesh.IsValid())
    return;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::BlockTillLoaded);

  for (auto itBone : pMesh->m_Bones)
  {
    const ezUInt16 uiJointIdx = msg.m_pSkeleton->FindJointByName(itBone.Key());

    if (uiJointIdx == ezInvalidJointIndex)
      continue;

    m_AnimTransforms[itBone.Value().m_uiBoneIndex] = msg.m_ModelTransforms[uiJointIdx] * itBone.Value().m_GlobalInverseBindPoseMatrix;
  }
}

void ezAnimatedMeshComponent::OnQueryAnimationSkeleton(ezMsgQueryAnimationSkeleton& msg)
{
  if (!msg.m_hSkeleton.IsValid())
  {
    // only overwrite, if no one else had a better skeleton (e.g. the ezSkeletonComponent)
    msg.m_hSkeleton = m_hSkeleton;
  }
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AnimatedMeshComponent);

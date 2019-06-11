#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/AnimatedMeshComponent.h>
#include <Interfaces/PhysicsWorldModule.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAnimatedMeshComponent, 10, ezComponentMode::Dynamic);
{
  EZ_BEGIN_PROPERTIES
  {
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
  SUPER::OnSimulationStarted();

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

    CreatePhysicsShapes(pSkeleton->GetDescriptor(), m_AnimationPose);

    m_AnimationPose.ConvertFromObjectSpaceToSkinningSpace(skeleton);

    CreateSkinningTransformBuffer(m_AnimationPose.GetAllTransforms());
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

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::AllowFallback);
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

  // inform child nodes/components that a new skinning pose is available
  // do this before the pose is transformed into skinning space
  {
    ezMsgAnimationPoseUpdated msg;
    msg.m_pSkeleton = &skeleton;
    msg.m_pPose = &m_AnimationPose;

    GetOwner()->SendMessageRecursive(msg);
  }

  m_AnimationPose.ConvertFromObjectSpaceToSkinningSpace(skeleton);

  UpdateSkinningTransformBuffer(m_AnimationPose.GetAllTransforms());

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

void ezAnimatedMeshComponent::CreatePhysicsShapes(const ezSkeletonResourceDescriptor& skeleton, const ezAnimationPose& pose)
{
  ezPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>();

  if (pPhysicsInterface == nullptr)
    return;

  // m_pRagdoll = pPhysicsInterface->CreateRagdoll(skeleton, GetOwner()->GetGlobalTransform(), pose);
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



EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_AnimatedMeshComponent);

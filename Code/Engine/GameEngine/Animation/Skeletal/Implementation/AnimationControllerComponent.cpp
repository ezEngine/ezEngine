#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <GameEngine/Animation/Skeletal/AnimationControllerComponent.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAnimationControllerComponent, 1, ezComponentMode::Static);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("AnimController", GetAnimationControllerFile, SetAnimationControllerFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Keyframe_Graph")),

    EZ_ENUM_MEMBER_PROPERTY("RootMotionMode", ezRootMotionMode, m_RootMotionMode),
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

ezAnimationControllerComponent::ezAnimationControllerComponent() = default;
ezAnimationControllerComponent::~ezAnimationControllerComponent() = default;

void ezAnimationControllerComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hAnimationController;
  s << m_RootMotionMode;
}

void ezAnimationControllerComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hAnimationController;
  s >> m_RootMotionMode;
}

void ezAnimationControllerComponent::SetAnimationControllerFile(const char* szFile)
{
  ezAnimGraphResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimGraphResource>(szFile);
  }

  m_hAnimationController = hResource;
}


const char* ezAnimationControllerComponent::GetAnimationControllerFile() const
{
  if (!m_hAnimationController.IsValid())
    return "";

  return m_hAnimationController.GetResourceID();
}

void ezAnimationControllerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (!m_hAnimationController.IsValid())
    return;

  ezMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  if (!msg.m_hSkeleton.IsValid())
    return;

  ezResourceLock<ezAnimGraphResource> pAnimController(m_hAnimationController, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimController.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  pAnimController->DeserializeAnimGraphState(m_AnimationGraph);

  m_AnimationGraph.Configure(msg.m_hSkeleton, m_PoseGenerator, ezBlackboardComponent::FindBlackboard(GetOwner()));
}

void ezAnimationControllerComponent::Update()
{
  m_AnimationGraph.Update(GetWorld()->GetClock().GetTimeDiff(), GetOwner());

  ezVec3 translation;
  ezAngle rotationX;
  ezAngle rotationY;
  ezAngle rotationZ;
  m_AnimationGraph.GetRootMotion(translation, rotationX, rotationY, rotationZ);

  ezRootMotionMode::Apply(m_RootMotionMode, GetOwner(), translation, rotationX, rotationY, rotationZ);
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AnimationControllerComponent);

#include <GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <GameEngine/Animation/Skeletal/AnimationControllerComponent.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <Physics/CharacterControllerComponent.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAnimationControllerComponent, 1, ezComponentMode::Static);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("AnimController", GetAnimationControllerFile, SetAnimationControllerFile)->AddAttributes(new ezAssetBrowserAttribute("Animation Controller")),

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

void ezAnimationControllerComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hAnimationController;
  s << m_RootMotionMode;
}

void ezAnimationControllerComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

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

  m_AnimationGraph.SetExternalBlackboard(ezBlackboardComponent::FindBlackboard(GetOwner()));

  m_AnimationGraph.m_hSkeleton = msg.m_hSkeleton;

  ezHashedString hs;
  hs.Assign("Idle");
  m_AnimationGraph.GetBlackboard().RegisterEntry(hs, 0.0f);
  hs.Assign("Left");
  m_AnimationGraph.GetBlackboard().RegisterEntry(hs, 0.0f);
  hs.Assign("Right");
  m_AnimationGraph.GetBlackboard().RegisterEntry(hs, 0.0f);
  hs.Assign("Forwards");
  m_AnimationGraph.GetBlackboard().RegisterEntry(hs, 0.0f);
  hs.Assign("Backwards");
  m_AnimationGraph.GetBlackboard().RegisterEntry(hs, 0.0f);
  hs.Assign("A");
  m_AnimationGraph.GetBlackboard().RegisterEntry(hs, 0.0f);
  hs.Assign("B");
  m_AnimationGraph.GetBlackboard().RegisterEntry(hs, 0.0f);
  hs.Assign("X");
  m_AnimationGraph.GetBlackboard().RegisterEntry(hs, 0.0f);
  hs.Assign("Y");
  m_AnimationGraph.GetBlackboard().RegisterEntry(hs, 0.0f);
}

void ezAnimationControllerComponent::Update()
{
  float fValue;
  bool bActive = false;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_NegX, &fValue);
  m_AnimationGraph.GetBlackboard().SetEntryValue("Left", fValue);
  bActive |= fValue != 0;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_PosX, &fValue);
  m_AnimationGraph.GetBlackboard().SetEntryValue("Right", fValue);
  bActive |= fValue != 0;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_NegY, &fValue);
  m_AnimationGraph.GetBlackboard().SetEntryValue("Backwards", fValue);
  bActive |= fValue != 0;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_PosY, &fValue);
  m_AnimationGraph.GetBlackboard().SetEntryValue("Forwards", fValue);
  bActive |= fValue != 0;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonA, &fValue);
  m_AnimationGraph.GetBlackboard().SetEntryValue("A", fValue);
  bActive |= fValue != 0;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonB, &fValue);
  m_AnimationGraph.GetBlackboard().SetEntryValue("B", fValue);
  bActive |= fValue != 0;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonX, &fValue);
  m_AnimationGraph.GetBlackboard().SetEntryValue("X", fValue);
  // bActive |= fValue != 0;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonY, &fValue);
  m_AnimationGraph.GetBlackboard().SetEntryValue("Y", fValue);
  bActive |= fValue != 0;

  m_AnimationGraph.GetBlackboard().SetEntryValue("Idle", bActive ? 0.0f : 1.0f);

  m_AnimationGraph.Update(GetWorld()->GetClock().GetTimeDiff(), GetOwner());

  const ezVec3 vRootMotion = m_AnimationGraph.GetRootMotion();

  float fRotate = 0;
  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_RightStick_NegX, &fValue);
  fRotate -= fValue;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_RightStick_PosX, &fValue);
  fRotate += fValue;

  auto pOwner = GetOwner();

  if (m_RootMotionMode != ezRootMotionMode::Ignore)
  {
    ezRootMotionMode::Apply(m_RootMotionMode, pOwner, vRootMotion);
  }
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AnimationControllerComponent);

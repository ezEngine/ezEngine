#include <GameEngine/GameEnginePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/World/GameObject.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/GameState/GameState.h>
#include <GameEngine/Gameplay/SceneTransitionComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSceneTransitionComponent, 1 /* version */, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("TargetScene", m_sTargetScene)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Scene",  ezDependencyFlags::Package))
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgTriggerTriggered, OnMsgTriggerTriggered),
  }
  EZ_END_MESSAGEHANDLERS;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSceneTransitionComponent::ezSceneTransitionComponent() = default;
ezSceneTransitionComponent::~ezSceneTransitionComponent() = default;

void ezSceneTransitionComponent::StartTransition()
{
  if (auto pGameStateBase = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    if (ezGameState* pGameState = ezDynamicCast<ezGameState*>(pGameStateBase))
    {
      pGameState->LoadScene(m_sTargetScene, {});
    }
  }
}

void ezSceneTransitionComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sTargetScene;
}

void ezSceneTransitionComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_sTargetScene;
}

void ezSceneTransitionComponent::OnMsgTriggerTriggered(ezMsgTriggerTriggered& msg)
{
  if (msg.m_TriggerState == ezTriggerState::Activated)
  {
    StartTransition();
  }
}

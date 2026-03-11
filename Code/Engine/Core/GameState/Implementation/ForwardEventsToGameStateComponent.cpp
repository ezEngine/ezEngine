#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/GameState/ForwardEventsToGameStateComponent.h>
#include <Core/GameState/GameStateBase.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezForwardEventsToGameStateComponent, 1 /* version */, ezComponentMode::Static)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezForwardEventsToGameStateComponent::ezForwardEventsToGameStateComponent() = default;
ezForwardEventsToGameStateComponent::~ezForwardEventsToGameStateComponent() = default;

bool ezForwardEventsToGameStateComponent::HandlesMessage(const ezMessage& msg) const
{
  // check whether there is any active game state
  // if so, test whether it would handle this type of message
  if (ezGameStateBase* pGameState = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->CanHandleMessage(msg.GetId());
  }

  return false;
}

bool ezForwardEventsToGameStateComponent::OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg)
{
  EZ_IGNORE_UNUSED(bWasPostedMsg);

  // if we have an active game state, forward the message to it
  if (ezGameStateBase* pGameState = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->DispatchMessage(pGameState, msg);
  }

  return false;
}

bool ezForwardEventsToGameStateComponent::OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg) const
{
  EZ_IGNORE_UNUSED(bWasPostedMsg);

  // if we have an active game state, forward the message to it
  if (const ezGameStateBase* pGameState = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->DispatchMessage(pGameState, msg);
  }

  return false;
}

void ezForwardEventsToGameStateComponent::Initialize()
{
  SUPER::Initialize();

  EnableUnhandledMessageHandler(true);
}


EZ_STATICLINK_FILE(Core, Core_GameState_Implementation_ForwardEventsToGameStateComponent);

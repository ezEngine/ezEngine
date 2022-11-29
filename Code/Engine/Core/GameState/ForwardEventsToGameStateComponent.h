#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>

using ezForwardEventsToGameStateComponentManager = ezComponentManager<class ezForwardEventsToGameStateComponent, ezBlockStorageType::Compact>;

/// \brief This event handler component forwards any message that it receives to the active ezGameStateBase.
///
/// Game states can have message handlers just like any other reflected type.
/// However, since they are not part of the ezWorld, messages are not delivered to them.
/// By attaching this component to a game object, all event messages that arrive at that node are
/// forwarded to the active game state. This way, a game state can receive information, such as
/// when a trigger gets activated.
///
/// Multiple of these components can exist in a scene, gathering and forwarding messages from many
/// different game objects, so that the game state can react to many different things.
class EZ_CORE_DLL ezForwardEventsToGameStateComponent : public ezEventMessageHandlerComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezForwardEventsToGameStateComponent, ezEventMessageHandlerComponent, ezForwardEventsToGameStateComponentManager);

public:
  //////////////////////////////////////////////////////////////////////////
  // ezForwardEventsToGameStateComponent

public:
  ezForwardEventsToGameStateComponent();
  ~ezForwardEventsToGameStateComponent();

protected:
  virtual bool HandlesEventMessage(const ezEventMessage& msg) const override;
  virtual bool OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg) override;
  virtual bool OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg) const override;

  virtual void Initialize() override;
};

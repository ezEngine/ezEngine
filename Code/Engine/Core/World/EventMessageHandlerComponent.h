#pragma once

#include <Core/World/World.h>

/// \brief Base class for components that want advanced handling of event messages.
///
/// Event messages are messages that are 'broadcast' to indicate something happened on a component,
/// e.g. a trigger that got activated or an animation that finished playing. These messages are 'bubbled up'
/// the object hierarchy to the closest parent object that holds a component that handles this message.
/// Event message handler components can control whether the search for handlers should be continued or
/// can register itself as global event message handlers which will be used if no handler is found in the parent hierarchy.
/// This is typically used for level-logic scripts that want to react to events happening on any object in the world
/// without needing to be attached to a specific object in the hierarchy.
class EZ_CORE_DLL ezEventMessageHandlerComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezEventMessageHandlerComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void Deinitialize() override;


  //////////////////////////////////////////////////////////////////////////
  // ezEventMessageHandlerComponent

public:
  /// \brief Keep the constructor private or protected in derived classes, so it cannot be called manually.
  ezEventMessageHandlerComponent();
  ~ezEventMessageHandlerComponent();

  /// \brief Sets the debug output object flag. The effect is type specific, most components will not do anything different.
  void SetDebugOutput(bool bEnable);

  /// \brief Gets the debug output object flag.
  bool GetDebugOutput() const;

  /// \brief Registers or de-registers this component as a global event handler.
  void SetGlobalEventHandlerMode(bool bEnable); // [ property ]

  /// \brief Returns whether this component is registered as a global event handler.
  bool GetGlobalEventHandlerMode() const { return m_bIsGlobalEventHandler; } // [ property ]

  /// \brief Sets whether unhandled event messages should be passed to parent objects or not.
  void SetPassThroughUnhandledEvents(bool bPassThrough);                               // [ property ]
  bool GetPassThroughUnhandledEvents() const { return m_bPassThroughUnhandledEvents; } // [ property ]

  /// \brief Returns all global event handler for the given world.
  static ezArrayPtr<ezComponentHandle> GetAllGlobalEventHandler(const ezWorld* pWorld);

  static void ClearGlobalEventHandlersForWorld(const ezWorld* pWorld);

private:
  bool m_bDebugOutput = false;
  bool m_bIsGlobalEventHandler = false;
  bool m_bPassThroughUnhandledEvents = false;
};

#pragma once

#include <Core/World/Component.h>

struct ezEventMessage;

class EZ_CORE_DLL ezEventMessageHandlerComponent : public ezComponent
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEventMessageHandlerComponent, ezComponent);

protected:
  /// \brief Keep the constructor private or protected in derived classes, so it cannot be called manually.
  ezEventMessageHandlerComponent();
  virtual ~ezEventMessageHandlerComponent();

  virtual void Deinitialize() override;

public:
  /// \brief Sets the debug output object flag. The effect is type specific, most components will not do anything different.
  void SetDebugOutput(bool enable);

  /// \brief Gets the debug output object flag.
  bool GetDebugOutput() const;

  /// \brief Registers or de-registers this component as a global event handler.
  void SetGlobalEventHandlerMode(bool enable);

  /// \brief Returns whether the given EventMessage is handled by this component.
  ///   If it is handled the sender will cache this component as receiver, if not the sender will not send this message anymore.
  virtual bool HandlesEventMessage(const ezEventMessage& msg) const;

  /// \brief Returns all global event handler for the given world.
  static ezArrayPtr<ezComponentHandle> GetAllGlobalEventHandler(const ezWorld* pWorld);

private:
  bool m_bDebugOutput;
  bool m_bIsGlobalEventHandler;
};


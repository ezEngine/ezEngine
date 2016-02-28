#pragma once

#include <GameUtils/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <Foundation/Time/Time.h>

/// \brief Message sent by ezTimedDeathComponent to itself to destroy it after the given timeout
struct EZ_GAMEUTILS_DLL ezTriggerTimedDeathMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezTriggerTimedDeathMessage);
};

typedef ezComponentManager<class ezTimedDeathComponent, true> ezTimedDeathComponentManager;

/// \brief This component deletes the object it is attached to after a timeout.
///
/// \note The timeout must be set immediately after component creation. Once the component
/// has been initialized (start of the next frame), changing the values has no effect.
/// The only way around this, is to delete the entire component and create a new one.
class EZ_GAMEUTILS_DLL ezTimedDeathComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezTimedDeathComponent, ezComponent, ezTimedDeathComponentManager);

public:
  ezTimedDeathComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  /// \brief Once this function has been executed, the timeout for deletion is fixed and cannot be reset.
  virtual Initialization Initialize() override;

  void OnDeathTriggered(ezTriggerTimedDeathMessage& msg) const;

  // ************************************* PROPERTIES ***********************************

  ezTime m_MinDelay;
  ezTime m_DelayRange;

private:


};

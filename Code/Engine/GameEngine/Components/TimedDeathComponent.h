#pragma once

#include <GameEngine/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <Foundation/Time/Time.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezComponentManager<class ezTimedDeathComponent, ezBlockStorageType::Compact> ezTimedDeathComponentManager;
typedef ezTypedResourceHandle<class ezPrefabResource> ezPrefabResourceHandle;

/// \brief This component deletes the object it is attached to after a timeout.
///
/// \note The timeout must be set immediately after component creation. Once the component
/// has been initialized (start of the next frame), changing the values has no effect.
/// The only way around this, is to delete the entire component and create a new one.
class EZ_GAMEENGINE_DLL ezTimedDeathComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezTimedDeathComponent, ezComponent, ezTimedDeathComponentManager);

public:
  ezTimedDeathComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  /// \brief Once this function has been executed, the timeout for deletion is fixed and cannot be reset.
  virtual void OnSimulationStarted() override;

  void OnTriggered(ezTriggerMessage& msg);

  // ************************************* PROPERTIES ***********************************

  ezTime m_MinDelay;
  ezTime m_DelayRange;

  void SetTimeoutPrefab(const char* szPrefab);
  const char* GetTimeoutPrefab() const;

private:

  ezPrefabResourceHandle m_hTimeoutPrefab; ///< Spawned when the component is killed due to the timeout



};

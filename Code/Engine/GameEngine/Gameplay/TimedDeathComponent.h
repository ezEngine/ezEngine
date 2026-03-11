#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Time/Time.h>
#include <GameEngine/GameEngineDLL.h>

struct ezMsgComponentInternalTrigger;
using ezTimedDeathComponentManager = ezComponentManager<class ezTimedDeathComponent, ezBlockStorageType::Compact>;
using ezPrefabResourceHandle = ezTypedResourceHandle<class ezPrefabResource>;

/// \brief This component deletes the object it is attached to after a timeout.
///
/// \note The timeout must be set immediately after component creation. Once the component
/// has been initialized (start of the next frame), changing the value has no effect.
/// The only way around this, is to delete the entire component and create a new one.
class EZ_GAMEENGINE_DLL ezTimedDeathComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezTimedDeathComponent, ezComponent, ezTimedDeathComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  /// \brief Once this function has been executed, the timeout for deletion is fixed and cannot be reset.
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // ezTimedDeathComponent

public:
  ezTimedDeathComponent();
  ~ezTimedDeathComponent();

  ezTime m_MinDelay = ezTime::MakeFromSeconds(1.0);   // [ property ]
  ezTime m_DelayRange = ezTime::MakeFromSeconds(0.0); // [ property ]

  ezPrefabResourceHandle m_hTimeoutPrefab;            ///< [ property ] Spawned when the component is killed due to the timeout

protected:
  void OnTriggered(ezMsgComponentInternalTrigger& msg);
};
